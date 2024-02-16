find_bare_dev(bare_dev)

list(APPEND fixtures
  addon
  bundle
)

foreach(fixture ${fixtures})
  execute_process(
    COMMAND "${bare_dev}" install --debug
    OUTPUT_QUIET
    ERROR_QUIET
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${fixture}"
    COMMAND_ERROR_IS_FATAL ANY
  )
endforeach()
