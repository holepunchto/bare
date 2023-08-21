cmake_minimum_required(VERSION 3.25)

project(bare C CXX)

set(BARE_USE_QUICKJS OFF CACHE BOOL "Use QuickJS instead of V8")

set(BARE_USE_JAVASCRIPTCORE OFF CACHE BOOL "Use JavaScriptCore instead of V8")

set(BARE_PREBUILDS_DIR ${PROJECT_SOURCE_DIR}/prebuilds CACHE PATH "Path to the prebuilds directory")

set(BARE_PREBUILDS_KEY ewr4dj5xxpec65ps915i7ckgzbugpssuw5xdtdaxgpxa9ixhbp8y CACHE STRING "Key of the prebuilds drive")

set(BARE_PREBUILDS_VERSION 2130 CACHE STRING "Version of the prebuilds drive")

include(bare)

bare_target(target)

add_bare_bundle(
  ENTRY src/bare.js
  OUT src/bare.js.h
  CONFIG src/bundle.config.js
  DEPENDS
    src/bare.js
    src/bundle.config.js
    src/import-map.json
    src/builtins/assert.js
    src/builtins/buffer.js
    src/builtins/console.js
    src/builtins/events.js
    src/builtins/globals.js
    src/builtins/module.js
    src/builtins/os.js
    src/builtins/path.js
    src/builtins/process.js
    src/builtins/process/addon.js
    src/builtins/process/thread.js
    src/builtins/thread.js
    src/builtins/timers.js
    src/builtins/url.js
)

add_bare_bundle(
  ENTRY bin/bare.js
  OUT bin/bare.bundle.h
  TARGET c
  DEPENDS
    bin/bare.js
)

mirror_drive(
  SOURCE ${BARE_PREBUILDS_KEY}
  DESTINATION ${BARE_PREBUILDS_DIR}
  PREFIX /${target}
  CHECKOUT ${BARE_PREBUILDS_VERSION}
)

if(MSVC)
  add_compile_options(
    $<$<CONFIG:Debug>:/MTd>
    $<$<CONFIG:Release>:/MT>
    /experimental:c11atomics
  )
endif()

if(NOT TARGET v8)
  add_library(v8 STATIC IMPORTED)

  find_library(
    v8
    NAMES v8 libv8
    PATHS ${BARE_PREBUILDS_DIR}/${target}/lib
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
      ${BARE_PREBUILDS_DIR}/${target}/include
  )
endif()

if(NOT TARGET uv)
  add_subdirectory(vendor/libuv EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET utf)
  add_subdirectory(vendor/libutf EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET js)
  add_subdirectory(vendor/libjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET qjs AND BARE_USE_QUICKJS)
  add_subdirectory(vendor/libqjs EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET jsc AND BARE_USE_JAVASCRIPTCORE)
  add_subdirectory(vendor/libjsc EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET napi)
  add_subdirectory(vendor/libnapi EXCLUDE_FROM_ALL)
endif()

if(BARE_USE_QUICKJS)
  set(engine qjs_static)
elseif(BARE_USE_JAVASCRIPTCORE)
  set(engine jsc_static)
else()
  set(engine js_static)
endif()

include_bare_module(bare_buffer node_modules/bare-buffer)
include_bare_module(bare_env node_modules/bare-env)
include_bare_module(bare_hrtime node_modules/bare-hrtime)
include_bare_module(bare_module node_modules/bare-module)
include_bare_module(bare_os node_modules/bare-os)
include_bare_module(bare_timers node_modules/bare-timers)

add_library(bare OBJECT)

set_target_properties(
  bare
  PROPERTIES
  C_STANDARD 11
  CXX_STANDARD 17
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  bare
  INTERFACE
    include/bare.h
    include/bare/modules.h
    include/bare/target.h
    include/bare/version.h
  PRIVATE
    src/addons.h
    src/bare.c
    src/bare.js.h
    src/runtime.h
    src/runtime/android.h
    src/runtime/posix.h
)

target_include_directories(
  bare
  PUBLIC
    include
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:js,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INTERFACE_INCLUDE_DIRECTORIES>
)

list(APPEND objects
  $<TARGET_OBJECTS:bare>
  $<TARGET_OBJECTS:bare_buffer>
  $<TARGET_OBJECTS:bare_env>
  $<TARGET_OBJECTS:bare_hrtime>
  $<TARGET_OBJECTS:bare_module>
  $<TARGET_OBJECTS:bare_os>
  $<TARGET_OBJECTS:bare_timers>
  $<TARGET_OBJECTS:uv>
  $<TARGET_OBJECTS:napi>
  $<TARGET_OBJECTS:mem>
  $<TARGET_OBJECTS:mimalloc>
  $<TARGET_OBJECTS:utf>
  $<TARGET_OBJECTS:base64>
  $<TARGET_OBJECTS:hex>
)

add_library(bare_shared SHARED ${objects})

set_target_properties(
  bare_shared
  PROPERTIES
  OUTPUT_NAME bare
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  bare_shared
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
)

add_library(bare_static STATIC ${objects})

set_target_properties(
  bare_static
  PROPERTIES
  OUTPUT_NAME bare
  PREFIX lib
)

target_link_libraries(
  bare_static
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
)

include_bare_module(bare_repl node_modules/bare-repl)
include_bare_module(bare_tty node_modules/bare-tty)

add_executable(bare_bin
  bin/bare.c
  bin/bare.bundle.h

  $<TARGET_OBJECTS:bare_repl>
  $<TARGET_OBJECTS:bare_tty>
)

set_target_properties(
  bare_bin
  PROPERTIES
  OUTPUT_NAME bare
  ENABLE_EXPORTS ON
  RUNTIME_OUTPUT_DIRECTORY bin
)

target_include_directories(
  bare_bin
  PRIVATE
    $<TARGET_PROPERTY:bare,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
  bare_bin
  PRIVATE
    $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
)

install(
  TARGETS bare_bin
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
