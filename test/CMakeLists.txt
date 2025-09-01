list(APPEND tests
  addon-load.js
  addon-load-unload.js
  addon-resolve.js
  argv-empty.c
  dependent-addon.js
  exit-on-before-exit.js
  exit-on-exit.js
  exit-on-idle.js
  exit-on-resume.js
  exit-on-suspend.js
  idle-on-idle.js
  idle-on-resume.js
  idle-on-suspend.js
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
  scoped-addon.js
  suspend-resume.js
  suspend-resume-on-idle.js
  suspend-resume-on-suspend.js
  suspend-resume-suspend.js
  suspend-resume-suspend-on-suspend.js
  suspend-resume-thread.js
  suspend-resume-with-linger.js
  thread.js
  thread-join.js
  thread-source.js
  thread-shared-data.js
  timeout.js
  timeout-clear.js
  wakeup.js
  wakeup-before-suspend.js
  wakeup-exceed-deadline.js
  wakeup-on-suspend.js
  wakeup-resume.js
  wakeup-suspend.js
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
  elseif(file MATCHES "\.(c|m)?js$")
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
      DISABLED ON
    )
  endif()
endforeach()
