cmake_minimum_required(VERSION 3.27)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-bare-bundle REQUIRED PATHS node_modules/cmake-bare-bundle)
find_package(cmake-drive REQUIRED PATHS node_modules/cmake-drive)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-napi REQUIRED PATHS node_modules/cmake-napi)

project(bare C CXX ASM)

bare_target(target)

mirror_drive(
  SOURCE qogbhqbcxknrpeotyz7hk4x3mxuf6d9mhb1dxm6ms5sdn6hh1uso
  DESTINATION ${PROJECT_SOURCE_DIR}/prebuilds
  PREFIX /${target}
  CHECKOUT 343
)

if(target MATCHES "win32")
  add_compile_options(/MT$<$<CONFIG:Debug>:d>)

  add_definitions(-DWIN32_LEAN_AND_MEAN)
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
        # Propagate the C++ STL dependency to all consumers that might end up
        # with the target in their link dependencies.
        /DEFAULTLIB:libcpmt$<$<CONFIG:Debug>:d>

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

  target_link_libraries(
    js
    INTERFACE
      v8
  )
endif()

fetch_package("github:libuv/libuv@1.50.0")
fetch_package("github:holepunchto/libutf#9509b28")
fetch_package("github:holepunchto/libjs#35e7566")
fetch_package("github:holepunchto/libnapi#324c25c")

add_bare_bundle(
  bare_bundle
  ENTRY src/bare.js
  OUT src/bare.js.h
  BUILTINS src/builtins.json
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
  PUBLIC
    include
)

target_link_libraries(
  bare
  INTERFACE
    $<TARGET_PROPERTY:uv,INTERFACE_LINK_LIBRARIES>
  PRIVATE
    $<TARGET_OBJECTS:utf>
    $<TARGET_OBJECTS:log>
    $<TARGET_OBJECTS:url>
    $<TARGET_OBJECTS:base64>
    $<TARGET_OBJECTS:hex>
  PUBLIC
    napi
  PRIVATE
    utf
    log
    url
    base64
    hex
)

link_bare_module(bare bare-buffer)
link_bare_module(bare bare-hrtime)
link_bare_module(bare bare-inspect)
link_bare_module(bare bare-logger)
link_bare_module(bare bare-module)
link_bare_module(bare bare-module-lexer)
link_bare_module(bare bare-os)
link_bare_module(bare bare-structured-clone)
link_bare_module(bare bare-timers)
link_bare_module(bare bare-type)
link_bare_module(bare bare-url)

add_library(bare_shared SHARED)

set_target_properties(
  bare_shared
  PROPERTIES
  OUTPUT_NAME bare
  ENABLE_EXPORTS ON
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

if(target MATCHES "win32")
  target_sources(
    bare_shared
    PUBLIC
      src/bare.def
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

if(target MATCHES "win32")
  target_sources(
    bare_static
    PUBLIC
      src/bare.def
      src/js.def
      src/napi.def
  )
endif()

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

add_subdirectory(bin)

install(TARGETS bare_static bare_shared)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
