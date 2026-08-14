# Runs a test that is expected to abort, inverting the pass condition.
#
# The output of the test is forwarded, which keeps `PASS_REGULAR_EXPRESSION`
# working for tests that also assert on what was reported.

if(arguments STREQUAL "")
  unset(arguments)
endif()

execute_process(
  COMMAND ${program} ${arguments}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)

if(NOT output STREQUAL "")
  message("${output}")
endif()

if(NOT error STREQUAL "")
  message("${error}")
endif()

if(result STREQUAL "0")
  message(FATAL_ERROR "Expected the test to abort, but it exited successfully")
endif()
