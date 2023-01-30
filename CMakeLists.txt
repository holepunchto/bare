cmake_minimum_required(VERSION 3.22)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(pear C CXX)

if(NOT DEFINED V8)
  set(V8 ${PROJECT_SOURCE_DIR}/prebuilds/v8/host/lib/libv8.a)
  set(V8_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/prebuilds/v8/host/include)
endif()

if(NOT TARGET uv)
  add_subdirectory(vendor/libuv EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET js)
  add_subdirectory(vendor/libjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET qjs)
  add_subdirectory(vendor/libqjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET jsc)
  add_subdirectory(vendor/libjsc EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET napi)
  add_subdirectory(vendor/libnapi EXCLUDE_FROM_ALL)
endif()

add_library(pear OBJECT)

set_target_properties(
  pear
  PROPERTIES
  C_STANDARD 99
  CXX_STANDARD 20
)

add_custom_command(
  COMMAND node scripts/link.mjs
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT
    bootstrap.h
    bootstrap.js
  DEPENDS
    lib/buffer/ascii.js
    lib/buffer/base64.js
    lib/buffer/hex.js
    lib/buffer/index.js
    lib/buffer/utf-8.js
    lib/bootstrap.js
    lib/load-addon.js
    lib/module.js
    lib/process.js
    lib/queue-microtask.js
  VERBATIM
)

target_sources(
  pear
  INTERFACE
    include/pearjs.h
  PRIVATE
    bootstrap.h
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

add_subdirectory(node_modules/tiny-timers-native EXCLUDE_FROM_ALL)

target_compile_definitions(
  tiny_timers_native
  PUBLIC
    NAPI_MODULE_FILENAME="/node_modules/tiny-timers-native"
)

add_executable(
  pear_bin
  bin/pear.c
  $<TARGET_OBJECTS:pear>
  $<TARGET_OBJECTS:uv>
  $<TARGET_OBJECTS:napi>
  $<TARGET_OBJECTS:tiny_timers_native>
)

set_target_properties(
  pear_bin
  PROPERTIES
  OUTPUT_NAME pear
  ENABLE_EXPORTS ON
)

target_include_directories(
  pear_bin
  PRIVATE
    $<TARGET_PROPERTY:pear,INCLUDE_DIRECTORIES>
)

target_link_libraries(
  pear_bin
  PRIVATE
    js_static
)

install(TARGETS pear_bin)
