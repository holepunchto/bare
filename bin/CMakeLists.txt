add_bare_bundle(
  bare_bin_bundle
  ENTRY bin/bare.js
  OUT bin/bare.bundle.h
  BUILTINS bin/builtins.json
  WORKING_DIRECTORY ..
)

add_executable(bare_bin
  bare.c
  bare.bundle.h
)

set_target_properties(
  bare_bin
  PROPERTIES
  OUTPUT_NAME bare
  ENABLE_EXPORTS ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  bare_bin
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
  PRIVATE
    rlimit_static
)

link_bare_module(bare_bin bare-crypto)
link_bare_module(bare_bin bare-dns)
link_bare_module(bare_bin bare-inspector)
link_bare_module(bare_bin bare-pipe)
link_bare_module(bare_bin bare-repl)
link_bare_module(bare_bin bare-signals)
link_bare_module(bare_bin bare-tcp)
link_bare_module(bare_bin bare-tls)
link_bare_module(bare_bin bare-tty)

target_link_libraries(ssl c++)

install(TARGETS bare_bin)
