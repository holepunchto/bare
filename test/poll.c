#include <assert.h>
#include <stdio.h>
#include <bare.h>
#include <js.h>
#include <string.h>
#include <uv.h>

// A process may be driven by a host loop that owns the thread, which polls it
// and then sleeps for as long as it was told to. Sleeping for longer than the
// process has left to wait would stall it, and being told to sleep for nothing
// at all would spin the host, so the test fails either way.

static const char *code =
  "let ticks = 0\n"
  "const interval = setInterval(() => {\n"
  "  if (++ticks === 3) clearInterval(interval)\n"
  "}, 50)\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, "/test.js", &source, NULL);
  assert(e == 0);

  uint64_t started = uv_hrtime();

  int polls = 0;

  while (true) {
    int timeout = -1;

    e = bare_poll(bare, &timeout);
    if (e == 0) break;

    polls++;

    // Nothing here waits on a descriptor, so the process should always know
    // how long it has left to wait.
    assert(timeout >= 0);

    fprintf(stderr, "poll %d: timeout=%d at %llums\n", polls, timeout, (unsigned long long) ((uv_hrtime() - started) / 1000000));

    uv_sleep(timeout);
  }

  uint64_t elapsed = (uv_hrtime() - started) / 1000000;

  // Three intervals of 50 ms, with room to spare for a loaded machine.
  assert(elapsed >= 150);
  assert(elapsed < 5000);

  // One poll per interval, give or take. Anything more means the host was
  // being woken for no reason.
  assert(polls < 50);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
