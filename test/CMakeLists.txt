list(APPEND tests
  import-addon.cjs
  import-addon.mjs
  import-bundle.js
  import-cjs.cjs
  import-cjs-from-mjs.mjs
  import-dynamic.mjs
  import-dynamic-from-cjs.cjs
  import-mjs.mjs
  import-mjs-from-cjs.cjs
  process-argv.js
  suspend-resume.js
  timeout.js
  timeout-clear.js
  user-data.c
)

add_subdirectory(fixtures)

foreach(file IN LISTS tests)
  get_filename_component(name ${file} NAME_WE)

  string(MAKE_C_IDENTIFIER ${name} test)

  if(file MATCHES "\.c$")
    add_executable(${test} ${file})

    target_link_libraries(
      ${test}
      PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,pear_static>
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
    TIMEOUT 30
  )
endforeach()
