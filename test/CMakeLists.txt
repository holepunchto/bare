list(APPEND tests
  addon.js
  argv-empty.c
  exit-on-before-exit.js
  exit-on-exit.js
  exit-on-idle.js
  exit-on-resume.js
  exit-on-suspend.js
  exit-on-wakeup.js
  import.cjs
  import.mjs
  restart.c
  suspension-idle.js
  suspension-wakeup.js
  suspension.js
  thread.js
  timeout.js
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
