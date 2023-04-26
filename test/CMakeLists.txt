list(APPEND tests
  argv-empty.c
  import-addon.cjs
  import-addon.mjs
  import-bundle.js
  import-cjs.cjs
  import-cjs.mjs
  import-dynamic-cjs.cjs
  import-dynamic-cjs.mjs
  import-dynamic-mjs.cjs
  import-dynamic-mjs.mjs
  import-mjs.cjs
  import-mjs.mjs
  process-addon.js
  process-addon-resolve.js
  process-argv.js
  process-chdir.js
  suspend-resume.js
  suspend-resume-immediate.js
  thread.js
  thread-source.js
  timeout.js
  timeout-clear.js
)

if(PEAR_USE_JAVASCRIPTCORE)
  list(APPEND skipped_tests
    import-addon.mjs
    import-cjs.mjs
    import-dynamic-cjs.cjs
    import-dynamic-cjs.mjs
    import-dynamic-mjs.cjs
    import-dynamic-mjs.mjs
    import-mjs.cjs
    import-mjs.mjs
  )
endif()

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

  if(${file} IN_LIST skipped_tests)
    set_tests_properties(
      ${file}
      PROPERTIES
      DISABLED True
    )
  endif()
endforeach()
