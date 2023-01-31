function(add_pear_module NAME PATH)
  execute_process(
    COMMAND pearjs-dev --require napi-macros --cwd ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE NAPI_MACROS_INCLUDE
  )

  execute_process(
    COMMAND pearjs-dev --include
    OUTPUT_VARIABLE PEAR_INCLUDE
  )

  add_library(${NAME} OBJECT)

  target_compile_definitions(
    ${NAME}
    PUBLIC
      NAPI_MODULE_FILENAME="/${PATH}"
  )

  target_include_directories(
    ${NAME}
    PUBLIC
      ${PEAR_INCLUDE}
      ${NAPI_MACROS_INCLUDE}
  )
endfunction()
