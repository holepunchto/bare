cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(pear C CXX)

execute_process(COMMAND v8-prebuilds-paths --lib OUTPUT_VARIABLE V8_PREBUILD_LIB)
execute_process(COMMAND v8-prebuilds-paths --include OUTPUT_VARIABLE V8_PREBUILD_INCLUDE)

set(V8 "${V8_PREBUILD_LIB}")
set(V8_INCLUDE_DIR "${V8_PREBUILD_INCLUDE}")

add_subdirectory(deps/libjs EXCLUDE_FROM_ALL)
add_subdirectory(deps/libnapi EXCLUDE_FROM_ALL)

include_directories(include)

add_executable(pear pear.c)

target_include_directories(
  pear
  PRIVATE
    $<TARGET_PROPERTY:js,INCLUDE_DIRECTORIES>
)

target_include_directories(
  pear
  PRIVATE
    $<TARGET_PROPERTY:napi,INCLUDE_DIRECTORIES>
)

target_link_libraries(
  pear
  PRIVATE
    js_static
)

