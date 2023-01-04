cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(pear C CXX)

set(V8 ${PROJECT_SOURCE_DIR}/prebuilds/v8/host/lib/libv8.a)
set(V8_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/prebuilds/v8/host/include)

add_subdirectory(deps/libjs EXCLUDE_FROM_ALL)
add_subdirectory(deps/libnapi EXCLUDE_FROM_ALL)

add_executable(pear pear.c)

target_include_directories(
  pear
  PRIVATE
    include
    $<TARGET_PROPERTY:js,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INCLUDE_DIRECTORIES>
)

target_link_libraries(
  pear
  PRIVATE
    js_static
    napi_static
)

