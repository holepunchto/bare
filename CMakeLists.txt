cmake_minimum_required(VERSION 3.25)

project(bare C)

set(BARE_USE_QUICKJS OFF CACHE BOOL "Use QuickJS instead of V8")

set(BARE_USE_JAVASCRIPTCORE OFF CACHE BOOL "Use JavaScriptCore instead of V8")

set(BARE_PREBUILDS_DIR ${PROJECT_SOURCE_DIR}/prebuilds CACHE PATH "Path to the prebuilds directory")

set(BARE_PREBUILDS_KEY jpf19qjy6b7rtqs8dg4d6cecu13u3m1ukej1kk4y873exh89wcxy CACHE STRING "Key of the prebuilds drive")

set(BARE_PREBUILDS_VERSION 77 CACHE STRING "Version of the prebuilds drive")

include(bare)

bare_target(target)

mirror_drive(
  SOURCE ${BARE_PREBUILDS_KEY}
  DESTINATION ${BARE_PREBUILDS_DIR}
  PREFIX /${target}
  CHECKOUT ${BARE_PREBUILDS_VERSION}
)

if(MSVC)
  add_compile_options(/MT$<$<CONFIG:Debug>:d>)

  add_link_options(/DEFAULTLIB:libcpmt$<$<CONFIG:Debug>:d>.lib)
endif()

if(NOT TARGET uv)
  add_subdirectory(vendor/libuv EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET utf)
  add_subdirectory(vendor/libutf EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET mem)
  add_subdirectory(vendor/libmem EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET c++)
  add_library(c++ STATIC IMPORTED)

  find_library(
    c++
    NAMES c++ libc++
    PATHS ${BARE_PREBUILDS_DIR}/${target}
    REQUIRED
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )

  set_target_properties(
    c++
    PROPERTIES
    IMPORTED_LOCATION ${c++}
  )
endif()

if(NOT TARGET v8)
  add_library(v8 STATIC IMPORTED)

  find_library(
    v8
    NAMES v8 libv8
    PATHS ${BARE_PREBUILDS_DIR}/${target}
    REQUIRED
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )

  set_target_properties(
    v8
    PROPERTIES
    IMPORTED_LOCATION ${v8}
  )

  target_link_libraries(
    v8
    INTERFACE
      c++
  )

  if(target MATCHES "linux")
    target_link_libraries(
      v8
      INTERFACE
        m
    )
  elseif(target MATCHES "win32")
    target_link_libraries(
      v8
      INTERFACE
        winmm.lib
    )
  endif()
endif()

if(NOT TARGET js)
  add_library(js STATIC IMPORTED)

  find_library(
    js
    NAMES js libjs
    PATHS ${BARE_PREBUILDS_DIR}/${target}
    REQUIRED
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )

  set_target_properties(
    js
    PROPERTIES
    IMPORTED_LOCATION ${js}
  )

  target_include_directories(
    js
    INTERFACE
      vendor/libjs/include
      vendor/libuv/include
      vendor/libutf/include
  )

  target_link_libraries(
    js
    INTERFACE
      v8
  )
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
  set(engine qjs)
elseif(BARE_USE_JAVASCRIPTCORE)
  set(engine jsc)
else()
  set(engine js)
endif()

add_bare_bundle(
  ENTRY src/bare.js
  OUT src/bare.js.h
  CONFIG src/bundle.config.js
)

add_bare_bundle(
  ENTRY bin/bare.js
  OUT bin/bare.bundle.h
  CONFIG bin/bundle.config.js
  DEPENDS
    bin/bare.js
    bin/bundle.config.js
)

add_compile_definitions(
  BARE_MODULE_REGISTER_CONSTRUCTOR
  NAPI_MODULE_REGISTER_CONSTRUCTOR
)

add_library(bare OBJECT)

set_target_properties(
  bare
  PROPERTIES
  C_STANDARD 11
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  bare
  INTERFACE
    include/bare.h
    include/bare/helper.h
    include/bare/module.h
    include/bare/target.h
    include/bare/version.h
  PRIVATE
    src/addon.c
    src/addon.h
    src/bare.c
    src/bare.js.h
    src/runtime.c
    src/runtime.h
    src/runtime/android.h
    src/runtime/posix.h
    src/thread.c
    src/thread.h
    src/types.h
)

target_include_directories(
  bare
  PUBLIC
    include
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:js,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INTERFACE_INCLUDE_DIRECTORIES>
)

include_bare_module(bare_buffer node_modules/bare-buffer)
include_bare_module(bare_env node_modules/bare-env)
include_bare_module(bare_hrtime node_modules/bare-hrtime)
include_bare_module(bare_module node_modules/bare-module)
include_bare_module(bare_os node_modules/bare-os)
include_bare_module(bare_timers node_modules/bare-timers)

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

  # Automatically export all available symbols on Windows.
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  bare_shared
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
  PRIVATE
    $<TARGET_PROPERTY:uv,INTERFACE_LINK_LIBRARIES>
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
  PRIVATE
    $<TARGET_PROPERTY:uv,INTERFACE_LINK_LIBRARIES>
)

include_bare_module(bare_repl node_modules/bare-repl)
include_bare_module(bare_tty node_modules/bare-tty)

add_executable(bare_bin ${objects}
  bin/bare.c
  bin/bare.bundle.h

  $<TARGET_OBJECTS:bare_repl>
  $<TARGET_OBJECTS:bare_tty>
)

set_target_properties(
  bare_bin
  PROPERTIES
  OUTPUT_NAME bare

  # Export symbols from the executable to allow native addons to link against
  # them.
  ENABLE_EXPORTS ON

  # Automatically export all available symbols on Windows.
  WINDOWS_EXPORT_ALL_SYMBOLS ON

  # Make sure none of the executable artefacts collide with the static and
  # shared library artefacts. This is particularly important on Windows.
  ARCHIVE_OUTPUT_DIRECTORY bin
  LIBRARY_OUTPUT_DIRECTORY bin
  RUNTIME_OUTPUT_DIRECTORY bin
)

target_include_directories(
  bare_bin
  PUBLIC
    $<TARGET_PROPERTY:bare,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
  bare_bin
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${engine}>
  PRIVATE
    $<TARGET_PROPERTY:uv,INTERFACE_LINK_LIBRARIES>
)

if(target MATCHES "android")
  find_library(log log)

  target_link_libraries(
    bare_bin
    PUBLIC
      ${log}
  )
endif()

install(
  TARGETS bare_bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION bin
)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()
