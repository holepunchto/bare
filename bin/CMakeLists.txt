include_bare_module(bare-pipe bare_pipe)
include_bare_module(bare-repl bare_repl)
include_bare_module(bare-tty bare_tty)

add_executable(bare_bin
  bare.c
  bare.bundle.h

  $<TARGET_OBJECTS:${bare_pipe}>
  $<TARGET_OBJECTS:${bare_repl}>
  $<TARGET_OBJECTS:${bare_tty}>
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

  # Make sure none of the executable artefacts collide with the static and
  # shared library artefacts. This is particularly important on Windows.
  ARCHIVE_OUTPUT_DIRECTORY bin
  LIBRARY_OUTPUT_DIRECTORY bin
  RUNTIME_OUTPUT_DIRECTORY bin
)

target_link_libraries(
  bare_bin
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
)

install(
  TARGETS bare_bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION bin
)
