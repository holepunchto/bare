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

  # Export symbols from the executable to allow native addons to link against
  # them.
  ENABLE_EXPORTS ON

  # Automatically export all available symbols on Windows.
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

if(target MATCHES "ios")
  set_target_properties(
    bare_bin
    PROPERTIES
    MACOSX_BUNDLE FALSE
  )
endif()

if(target MATCHES "win32")
  target_link_options(
    bare_bin
    PRIVATE
      /NOIMPLIB
  )
endif()

target_link_libraries(
  bare_bin
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
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

install(TARGETS bare_bin)
