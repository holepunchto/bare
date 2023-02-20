cmake_minimum_required(VERSION 3.25)

project(pear C CXX)

option(PEAR_USE_QUICKJS OFF)
option(PEAR_USE_JAVASCRIPTCORE OFF)

include(pear)

pear_target(target)

find_program(npx NAMES npx REQUIRED)

set(prebuilds_dir ${PROJECT_SOURCE_DIR}/prebuilds)
set(prebuilds_key gy9t1gat1umpqo8bm5z7a4j38xm1g1z59zid69rq6quxzg3ij8uo)

add_custom_command(
  COMMAND ${npx} pear-dev bundle --config bundle.config.js src/pear.js
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT
    ${PROJECT_SOURCE_DIR}/src/pear.js.h
  DEPENDS
    src/pear.js
    src/builtins/assert.js
    src/builtins/buffer.js
    src/builtins/console.js
    src/builtins/events.js
    src/builtins/module.js
    src/builtins/path.js
    src/builtins/process.js
    src/builtins/timers.js
  VERBATIM
)

add_custom_command(
  COMMAND ${npx} pear-dev bundle --out bin/pear.bundle.h --target c bin/pear.js
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT
    ${PROJECT_SOURCE_DIR}/bin/pear.bundle.h
  DEPENDS
    bin/pear.js
  VERBATIM
)

add_custom_command(
  COMMAND ${npx} drives mirror --prefix /${target} ${prebuilds_key} ${prebuilds_dir}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT
    ${prebuilds_dir}/${target}/lib/libv8.a
  VERBATIM
)

add_custom_target(
  pear_prebuilds
  DEPENDS
    ${prebuilds_dir}/${target}/lib/libv8.a
)

file(MAKE_DIRECTORY ${prebuilds_dir}/${target}/include)
file(MAKE_DIRECTORY ${prebuilds_dir}/${target}/lib)

if(NOT TARGET v8)
  add_library(v8 STATIC IMPORTED)

  add_dependencies(v8 pear_prebuilds)

  set_target_properties(
    v8
    PROPERTIES
    IMPORTED_LOCATION ${prebuilds_dir}/${target}/lib/libv8.a
  )

  target_include_directories(
    v8
    INTERFACE
      ${prebuilds_dir}/${target}/include
  )
endif()

if(NOT TARGET uv)
  add_subdirectory(vendor/libuv EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET js)
  add_subdirectory(vendor/libjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET qjs AND PEAR_USE_QUICKJS)
  add_subdirectory(vendor/libqjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET jsc AND PEAR_USE_JAVASCRIPTCORE)
  add_subdirectory(vendor/libjsc EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET napi)
  add_subdirectory(vendor/libnapi EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET path)
  add_subdirectory(vendor/libpath EXCLUDE_FROM_ALL)
endif()

add_dependencies(js pear_prebuilds)

if(PEAR_USE_QUICKJS)
  set(engine qjs_static)
elseif(PEAR_USE_JAVASCRIPTCORE)
  set(engine jsc_static)
else()
  set(engine js_static)
endif()

include_pear_module(pear_buffer node_modules/@pearjs/buffer)
include_pear_module(pear_module node_modules/@pearjs/module)
include_pear_module(pear_timers node_modules/@pearjs/timers)

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
    src/addons.h
    src/fs.h
    src/pear.c
    src/pear.js.h
    src/runtime.h
)

target_include_directories(
  pear
  PUBLIC
    include
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:js,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:path,INTERFACE_INCLUDE_DIRECTORIES>
)

list(APPEND objects
  $<TARGET_OBJECTS:pear>
  $<TARGET_OBJECTS:pear_buffer>
  $<TARGET_OBJECTS:pear_module>
  $<TARGET_OBJECTS:pear_timers>
  $<TARGET_OBJECTS:uv>
  $<TARGET_OBJECTS:napi>
  $<TARGET_OBJECTS:path>
)

add_library(pear_shared SHARED ${objects})

set_target_properties(
  pear_shared
  PROPERTIES
  OUTPUT_NAME pear
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  pear_shared
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
)

add_library(pear_static STATIC ${objects})

set_target_properties(
  pear_static
  PROPERTIES
  OUTPUT_NAME pear
  PREFIX lib
)

target_link_libraries(
  pear_static
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
)

add_executable(pear_bin bin/pear.c bin/pear.bundle.h)

set_target_properties(
  pear_bin
  PROPERTIES
  OUTPUT_NAME pear
  ENABLE_EXPORTS ON
  RUNTIME_OUTPUT_DIRECTORY bin
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

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()
