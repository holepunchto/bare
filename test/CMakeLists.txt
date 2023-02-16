list(APPEND tests
  import-addon.cjs
  import-addon.mjs
  import-cjs.cjs
  import-cjs-from-mjs.mjs
  import-mjs.mjs
  process-argv.js
  timeout.js
  timeout-clear.js
)

add_subdirectory(fixtures/addon)

foreach(file IN LISTS tests)
  get_filename_component(name ${file} NAME_WE)

  string(MAKE_C_IDENTIFIER ${name} test)

  if(file MATCHES "\.c$")
    add_executable(${test} ${file})

    set_target_properties(
      ${test}
      PROPERTIES
      OUTPUT_NAME test_${test}
    )

    target_link_libraries(
      ${test}
      PRIVATE
        pear_static
    )

    target_include_directories(
      ${test}
      PRIVATE
        $<TARGET_PROPERTY:pear,INTERFACE_INCLUDE_DIRECTORIES>
    )

    set(command ${test})
  endif()

  if(file MATCHES "\.(c|m)?js$")
    set(command pear_bin test/${file})
  endif()

  add_test(
    NAME ${file}
    COMMAND ${command}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  set_tests_properties(
    ${file}
    PROPERTIES
    TIMEOUT 30000
  )
endforeach()
