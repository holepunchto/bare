cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(pear C CXX)

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
