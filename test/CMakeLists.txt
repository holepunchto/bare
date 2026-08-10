list(APPEND tests
  addon-constructor.js
  addon-constructor-multiple.js
  addon-constructor-multiple-processes.c
  addon-constructor-multiple-threads.js
  addon-dependent.js
  addon-invalid-file-url.js
  addon-multiple-processes.c
  addon-multiple-threads.js
  addon-multiple.js
  addon-scoped.js
  addon-seal.js
  addon-seal-multiple-threads.js
  addon-seal-preloaded-multiple-threads.js
  addon-seal-restart.c
  addon-teardown.c
  addon-teardown-multiple-processes.c
  addon.js
  argv-empty.c
  exit-enqueue-immediate.js
  exit-enqueue-readfile.js
  exit-enqueue-timeout.js
  exit-flush-microtask.js
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
  import-cts.cts
  import-mts.mts
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

if(WIN32)
  list(APPEND skipped_tests
    # The teardown addon observes being unloaded through a destructor that
    # records it in a host symbol resolved at load time. This excludes Windows,
    # where a prebuilt addon can only import the symbols Bare itself exports.
    addon-teardown.c
    addon-teardown-multiple-processes.c
  )
endif()

foreach(file IN LISTS tests)
  get_filename_component(name ${file} NAME_WE)

  string(MAKE_C_IDENTIFIER ${name} test)

  if(file MATCHES "\.c$")
    add_executable(${test} ${file})

    set_target_properties(
      ${test}
      PROPERTIES
      ENABLE_EXPORTS ON
      WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_link_libraries(
      ${test}
      PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
    )

    set(command ${test})
  elseif(file MATCHES "\.(c|m)?[jt]s$")
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
