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

add_executable(bare::bin ALIAS bare_bin)

if(WIN32)
  target_sources(bare_bin PRIVATE bare.manifest)
endif()

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

if(target MATCHES "darwin|ios")
  target_link_libraries(
    bare_bin
    PRIVATE
      bare_exports
  )

  target_link_options(
    bare_bin
    PRIVATE
      $<$<CONFIG:Release>:-Wl,-dead_strip,-S,-x>
  )
elseif(target MATCHES "linux|android")
  set_target_properties(
    bare_bin
    PROPERTIES
    ENABLE_EXPORTS OFF
    LINK_DEPENDS "${dynamic_list_file}"
  )

  target_link_options(
    bare_bin
    PRIVATE
      "-Wl,--dynamic-list=${dynamic_list_file}"
      $<$<CONFIG:Release>:-Wl,--gc-sections,--strip-all>
  )
endif()

link_bare_module(bare_bin bare-crypto)
link_bare_module(bare_bin bare-dns)
link_bare_module(bare_bin bare-inspector)
link_bare_module(bare_bin bare-os)
link_bare_module(bare_bin bare-path)
link_bare_module(bare_bin bare-pipe)
link_bare_module(bare_bin bare-repl)
link_bare_module(bare_bin bare-signals)
link_bare_module(bare_bin bare-tcp)
link_bare_module(bare_bin bare-tls)
link_bare_module(bare_bin bare-tty)

if(TARGET c++)
  target_link_libraries(ssl c++)
endif()

install(TARGETS bare_bin)
