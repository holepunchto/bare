list(APPEND tests
  addon-load.js
  addon-resolve.js
  argv-empty.c
  exit-on-before-exit.js
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
  restart.c
  suspend-from-thread.js
  suspend-resume.js
  suspend-resume-from-thread.js
  suspend-resume-on-idle.js
  suspend-resume-on-suspend.js
  thread.js
  thread-join.js
  thread-source.js
  thread-shared-data.js
  timeout.js
  timeout-clear.js
)

foreach(file IN LISTS tests)
  get_filename_component(name ${file} NAME_WE)

  string(MAKE_C_IDENTIFIER ${name} test)

  if(file MATCHES "\.c$")
    add_executable(${test} ${file})

    target_link_libraries(
      ${test}
      PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
    )

    set(command ${test})
  endif()

  if(file MATCHES "\.(c|m)?js$")
    set(command bare_bin test/${file})
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
