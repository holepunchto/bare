list(APPEND tests
  addon-load-dependent.js
  addon-load-multiple-threads.js
  addon-load-scoped.js
  addon-load.js
  addon-resolve.js
  argv-empty.c
  exit-on-before-exit.js
  exit-on-exit.js
  exit-on-idle.js
  exit-on-resume.js
  exit-on-suspend.js
  exit-on-wakeup.js
  idle-before-suspend.js
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
  suspend-idle.js
  suspend-on-idle.js
  suspend-resume-on-idle.js
  suspend-resume-on-suspend.js
  suspend-resume-suspend-on-suspend.js
  suspend-resume-suspend-with-linger.js
  suspend-resume-suspend.js
  suspend-resume-thread.mjs
  suspend-resume-with-linger.js
  suspend-resume.js
  suspend-suspend-resume-thread.mjs
  thread-join.js
  thread-shared-data.js
  thread-source.js
  thread-transfer-data.js
  thread.js
  wakeup-before-suspend.js
  wakeup-exceed-deadline.js
  wakeup-idle-on-wakeup.js
  wakeup-on-idle.js
  wakeup-on-resume.js
  wakeup-on-suspend.js
  wakeup-on-wakeup.js
  wakeup-resume-on-wakeup.js
  wakeup-resume.js
  wakeup-suspend-on-wakeup.js
  wakeup-suspend-resume-on-wakeup.js
  wakeup-with-linger-thread.mjs
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
