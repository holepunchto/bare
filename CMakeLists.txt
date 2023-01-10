cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(pear C CXX)

if(NOT DEFINED V8)
  set(V8 ${PROJECT_SOURCE_DIR}/prebuilds/v8/host/lib/libv8.a)
  set(V8_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/prebuilds/v8/host/include)
endif()

if(NOT TARGET js)
  add_subdirectory(deps/libjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET napi)
  add_subdirectory(deps/libnapi EXCLUDE_FROM_ALL)
endif()

add_library(pear OBJECT)

set_target_properties(
  pear
  PROPERTIES
  C_STANDARD 99
  CXX_STANDARD 20
)

target_sources(
  pear
  INTERFACE
    include/pearjs.h
  PRIVATE
    src/pear.c
    src/addons.c
    src/sync_fs.c
    src/runtime.c
)

target_include_directories(
  pear
  PUBLIC
    include
    $<TARGET_PROPERTY:uv,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:js,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INCLUDE_DIRECTORIES>
)

add_executable(pear_bin $<TARGET_OBJECTS:pear> $<TARGET_OBJECTS:uv>)

set_target_properties(
  pear_bin
  PROPERTIES
  OUTPUT_NAME pear
  ENABLE_EXPORTS ON
)

target_link_libraries(
  pear_bin
  PRIVATE
    js_static
    napi_static
)
