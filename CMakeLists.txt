cmake_minimum_required(VERSION 3.25)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

project(pear C CXX)

option(PEAR_ENABLE_ASAN OFF)

option(PEAR_USE_QUICKJS OFF)
option(PEAR_USE_JAVASCRIPTCORE OFF)

include(pear)

find_program(node NAMES node)

set(prebuilds_dir ${PROJECT_SOURCE_DIR}/prebuilds)

add_custom_command(
  COMMAND ${node} scripts/link.mjs
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT
    ${PROJECT_SOURCE_DIR}/src/bootstrap.js
    ${PROJECT_SOURCE_DIR}/src/bootstrap.js.h
  DEPENDS
    lib/bootstrap.js
    lib/process.js
  VERBATIM
)

add_custom_command(
  COMMAND ${node} scripts/fetch.mjs
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT
    ${prebuilds_dir}/host/lib/libv8.a
  VERBATIM
)

add_custom_target(
  pear_prebuilds
  DEPENDS
    ${prebuilds_dir}/host/lib/libv8.a
)

file(MAKE_DIRECTORY ${prebuilds_dir}/host/include)

if(NOT TARGET v8)
  add_library(v8 STATIC IMPORTED)

  add_dependencies(v8 pear_prebuilds)

  set_target_properties(
    v8
    PROPERTIES
    IMPORTED_LOCATION ${prebuilds_dir}/host/lib/libv8.a
  )

  target_include_directories(
    v8
    INTERFACE
      ${prebuilds_dir}/host/include
  )
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

add_dependencies(js pear_prebuilds)

add_library(pear OBJECT)

set_target_properties(
  pear
  PROPERTIES
  C_STANDARD 99
  CXX_STANDARD 17
)

target_sources(
  pear
  INTERFACE
    include/pear.h
    include/pear/modules.h
  PRIVATE
    src/addons.c
    src/bootstrap.js.h
    src/fs.c
    src/pear.c
    src/runtime.c
)

target_include_directories(
  pear
  PUBLIC
    include
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:js,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INTERFACE_INCLUDE_DIRECTORIES>
)

include_pear_module(pear_buffer node_modules/@pearjs/buffer)
include_pear_module(pear_timers node_modules/@pearjs/timers)

add_library(
  pear_static
  STATIC
  $<TARGET_OBJECTS:pear>
  $<TARGET_OBJECTS:pear_buffer>
  $<TARGET_OBJECTS:pear_timers>
  $<TARGET_OBJECTS:napi>
)

set_target_properties(
  pear_static
  PROPERTIES
  OUTPUT_NAME pear
)

set(engine js_static)

if(PEAR_USE_QUICKJS)
  set(engine qjs_static)
endif()

if(PEAR_USE_JAVASCRIPTCORE)
  set(engine jsc_static)
endif()

target_link_libraries(
  pear_static
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
)

add_executable(pear_bin bin/pear.c)

add_debug_options(pear_bin)

set_target_properties(
  pear_bin
  PROPERTIES
  OUTPUT_NAME pear
  ENABLE_EXPORTS ON
)

target_include_directories(
  pear_bin
  PRIVATE
    $<TARGET_PROPERTY:pear,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
  pear_bin
  PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,pear_static>
)

install(
  TARGETS pear_bin
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION bin
)
