cmake_minimum_required(VERSION 3.25)

project(pear C CXX)

set(PEAR_USE_QUICKJS OFF CACHE BOOL "Use QuickJS instead of V8")

set(PEAR_USE_JAVASCRIPTCORE OFF CACHE BOOL "Use JavaScriptCore instead of V8")

set(PEAR_PREBUILDS_DIR ${PROJECT_SOURCE_DIR}/prebuilds CACHE PATH "Path to the prebuilds directory")

set(PEAR_PREBUILDS_KEY ewr4dj5xxpec65ps915i7ckgzbugpssuw5xdtdaxgpxa9ixhbp8y CACHE STRING "Key of the prebuilds drive")

set(PEAR_PREBUILDS_VERSION 1702 CACHE STRING "Version of the prebuilds drive")

include(pear)

pear_target(target)

add_pear_bundle(
  ENTRY src/pear.js
  OUT src/pear.js.h
  CONFIG bundle.config.js
  DEPENDS
    bundle.config.js
    src/pear.js
    src/builtins/assert.js
    src/builtins/buffer.js
    src/builtins/console.js
    src/builtins/events.js
    src/builtins/module.js
    src/builtins/path.js
    src/builtins/process.js
    src/builtins/process/addon.js
    src/builtins/process/data.js
    src/builtins/process/env.js
    src/builtins/process/hrtime.js
    src/builtins/timers.js
)

add_pear_bundle(
  ENTRY bin/pear.js
  OUT bin/pear.bundle.h
  TARGET c
  DEPENDS
    bin/pear.js
)

mirror_drive(
  SOURCE ${PEAR_PREBUILDS_KEY}
  DESTINATION ${PEAR_PREBUILDS_DIR}
  PREFIX /${target}
  CHECKOUT ${PEAR_PREBUILDS_VERSION}
)

if(MSVC)
  add_compile_options($<$<CONFIG:Debug>:/MTd> $<$<CONFIG:Release>:/MT>)
endif()

if(NOT TARGET v8)
  add_library(v8 STATIC IMPORTED)

  find_library(
    v8
    NAMES v8 libv8
    PATHS ${PEAR_PREBUILDS_DIR}/${target}/lib
    REQUIRED
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )

  set_target_properties(
    v8
    PROPERTIES
    IMPORTED_LOCATION ${v8}
  )

  target_include_directories(
    v8
    INTERFACE
      ${PEAR_PREBUILDS_DIR}/${target}/include
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
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  pear
  INTERFACE
    include/pear.h
    include/pear/modules.h
    include/pear/target.h
    include/pear/version.h
  PRIVATE
    src/addons.h
    src/pear.c
    src/pear.js.h
    src/runtime.h
    src/runtime/android.h
    src/runtime/posix.h
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
  $<TARGET_OBJECTS:mem>
  $<TARGET_OBJECTS:mimalloc>
  $<TARGET_OBJECTS:path>
  $<TARGET_OBJECTS:base64>
  $<TARGET_OBJECTS:hex>
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
  RUNTIME
    DESTINATION bin
    COMPONENT core
  BUNDLE
    DESTINATION bin
    COMPONENT core
)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()
