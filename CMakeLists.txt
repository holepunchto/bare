cmake_minimum_required(VERSION 3.27)

project(bare C)

include(bare)

bare_target(target)

include(cmake/options.cmake)
include(cmake/overrides.cmake)

install_node_modules(LOCKFILE)

mirror_drive(
  SOURCE qogbhqbcxknrpeotyz7hk4x3mxuf6d9mhb1dxm6ms5sdn6hh1uso
  DESTINATION ${PROJECT_SOURCE_DIR}/prebuilds
  PREFIX /${target}
  CHECKOUT 212
)

if(target MATCHES "win32")
  add_compile_options(/MT$<$<CONFIG:Debug>:d>)

  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

if(NOT TARGET uv)
  add_subdirectory(vendor/libuv EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET utf)
  add_subdirectory(vendor/libutf EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET c++)
  add_library(c++ STATIC IMPORTED GLOBAL)

  find_library(
    c++
    NAMES c++ libc++
    PATHS ${PROJECT_SOURCE_DIR}/prebuilds/${target}
    REQUIRED
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )

  set_target_properties(
    c++
    PROPERTIES
    IMPORTED_LOCATION ${c++}
  )

  if(target MATCHES "win32")
    target_link_options(
      c++
      INTERFACE
        # The imported static library always links the release version of the
        # static CRT; explicitly tell MSVC to not link the release version for
        # debug builds and vice versa.
        /NODEFAULTLIB:libcmt$<$<CONFIG:Release>:d>
    )
  endif()
endif()

if(NOT TARGET v8)
  add_library(v8 STATIC IMPORTED GLOBAL)

  find_library(
    v8
    NAMES v8 libv8
    PATHS ${PROJECT_SOURCE_DIR}/prebuilds/${target}
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
  elseif(target MATCHES "android")
    find_library(log log)

    target_link_libraries(
      v8
      INTERFACE
        ${log}
    )
  elseif(target MATCHES "win32")
    target_link_libraries(
      v8
      INTERFACE
        winmm
    )
  endif()
endif()

if(NOT TARGET js)
  add_library(js STATIC IMPORTED GLOBAL)

  find_library(
    js
    NAMES js libjs
    PATHS ${PROJECT_SOURCE_DIR}/prebuilds/${target}
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

if(NOT TARGET napi)
  add_subdirectory(vendor/libnapi EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET log)
  add_subdirectory(vendor/liblog EXCLUDE_FROM_ALL)
endif()

add_bare_bundle(
  ENTRY src/bare.js
  OUT src/bare.js.h
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
  PUBLIC
    $<TARGET_OBJECTS:uv>
    $<TARGET_OBJECTS:napi>
    $<TARGET_OBJECTS:utf>
    $<TARGET_OBJECTS:url>
    $<TARGET_OBJECTS:base64>
    $<TARGET_OBJECTS:hex>
    $<TARGET_OBJECTS:log>
  PRIVATE
    src/addon.c
    src/addon.h
    src/bare.c
    src/bare.js.h
    src/runtime.c
    src/runtime.h
    src/thread.c
    src/thread.h
    src/types.h
)

target_include_directories(
  bare
  INTERFACE
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:js,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:napi,INTERFACE_INCLUDE_DIRECTORIES>
  PUBLIC
    include
)

target_link_libraries(
  bare
  INTERFACE
    $<TARGET_PROPERTY:uv,INTERFACE_LINK_LIBRARIES>
  PUBLIC
    napi
    utf
    url
    base64
    hex
    log
)

target_compile_definitions(
  bare
  PUBLIC
    BARE_USE_SYSTEM_LOG=$<BOOL:${BARE_USE_SYSTEM_LOG}>
)

if(target MATCHES "win32")
  target_compile_options(
    bare
    PRIVATE
      /experimental:c11atomics
  )
endif()

add_library(bare_shared SHARED)

set_target_properties(
  bare_shared
  PROPERTIES
  OUTPUT_NAME bare

  # Export an import library for supported platforms, namely macOS and iOS.
  ENABLE_EXPORTS ON

  # Automatically export all available symbols on Windows.
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

if(target MATCHES "win32")
  target_sources(
    bare_shared
    PUBLIC
      src/js.def
      src/napi.def
  )
endif()

target_include_directories(
  bare_shared
  INTERFACE
    $<TARGET_PROPERTY:bare,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
  bare_shared
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,js>
  PRIVATE
    bare
)

add_library(bare_static STATIC)

set_target_properties(
  bare_static
  PROPERTIES
  OUTPUT_NAME bare
  PREFIX lib
)

target_include_directories(
  bare_static
  INTERFACE
    $<TARGET_PROPERTY:bare,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(
  bare_static
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,js>
  PRIVATE
    bare
)

if(target MATCHES "win32")
  target_sources(
    bare_static
    PUBLIC
      src/js.def
      src/napi.def
  )
endif()

add_bare_bundle(
  ENTRY bin/bare.js
  OUT bin/bare.bundle.h
)

add_executable(bare_bin
  bin/bare.c
  bin/bare.bundle.h
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

if(target MATCHES "ios")
  set_target_properties(
    bare_bin
    PROPERTIES
    MACOSX_BUNDLE FALSE
  )
endif()

target_link_libraries(
  bare_bin
  PUBLIC
    $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
)

link_bare_module(bare bare-buffer)
link_bare_module(bare bare-hrtime)
link_bare_module(bare bare-inspect)
link_bare_module(bare bare-module)
link_bare_module(bare bare-os)
link_bare_module(bare bare-structured-clone)
link_bare_module(bare bare-timers)
link_bare_module(bare bare-type)
link_bare_module(bare bare-url)

link_bare_module(bare_bin bare-pipe)
link_bare_module(bare_bin bare-repl)
link_bare_module(bare_bin bare-signals)
link_bare_module(bare_bin bare-crypto)
link_bare_module(bare_bin bare-dns)
link_bare_module(bare_bin bare-inspector)
link_bare_module(bare_bin bare-tcp)
link_bare_module(bare_bin bare-tls)
link_bare_module(bare_bin bare-tty)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
