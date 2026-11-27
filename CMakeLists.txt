cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-harden REQUIRED PATHS node_modules/cmake-harden)
find_package(cmake-bare-bundle REQUIRED PATHS node_modules/cmake-bare-bundle)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-napi REQUIRED PATHS node_modules/cmake-napi)
find_package(cmake-prelude REQUIRED PATHS node_modules/cmake-prelude)
find_package(cmake-test REQUIRED PATHS node_modules/cmake-test)

project(bare LANGUAGES C CXX VERSION 1.39.0)

bare_target(target)

if(target MATCHES "win32-x64")
  enable_language(ASM_NASM)
else()
  enable_language(ASM)
endif()

if(target MATCHES "freertos")
  set(hosted OFF)
else()
  set(hosted ON)
endif()

set(BARE_ENGINE "github:holepunchto/libjs#905a61a" CACHE STRING "The JavaScript engine to use")

set(BARE_MEMORY_LIMIT 0 CACHE STRING "The default memory limit of each JavaScript heap")

set(BARE_DYNAMIC_ADDONS ${hosted} CACHE BOOL "Enable loading addons dynamically at runtime")

set(BARE_FILE_SYSTEM_MODULES ${hosted} CACHE BOOL "Enable loading modules from the file system")

set(BARE_CLI ${hosted} CACHE BOOL "Build the command line interface")

get_property(shared GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)

if(NOT shared)
  set(LIBUV_BUILD_SHARED OFF CACHE BOOL "Build shared lib")
endif()

fetch_package("github:libuv/libuv@1.53.0")

if(NOT shared)
  add_library(uv ALIAS uv_a)
endif()

fetch_package("github:holepunchto/librlimit#a19e5d0")
fetch_package("github:holepunchto/libutf#b22ffb0")

fetch_package(${BARE_ENGINE})

fetch_package("github:holepunchto/libnapi#ce2686a")

get_target_property(js_exports js::exports INTERFACE_EXPORTED_SYMBOLS)
get_target_property(napi_exports napi::exports INTERFACE_EXPORTED_SYMBOLS)

add_bare_bundle(
  bare_bundle
  ENTRY src/bare.js
  OUT src/bare.js.h
  BUILTINS src/builtins.json
  COMPRESSED
)

configure_file(include/bare/version.h.in include/bare/version.h @ONLY)

add_library_targets(
  bare
  HEADERS
    include/bare.h
    include/node_api.h
    include/bare/helper.h
    include/bare/module.h
    include/bare/target.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/bare/version.h"
  BASE_DIRS
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include"
  FEATURES
    c_std_11
  SOURCES
    src/addon.c
    src/addon.h
    src/bare.c
    src/bare.js.h
    src/context.c
    src/context.h
    src/runtime.c
    src/runtime.h
    src/thread.c
    src/thread.h
    src/types.h
  LINK
    js::headers
    napi::headers
    PRIVATE
      js::static
      napi::static
    EMBED
      uv
  EXPORTS
    ${js_exports}
    ${napi_exports}
  EXPORTS_FROM
    src/exports.json
  EXPORT OFF
)

harden(bare)

target_compile_definitions(
  bare
  PRIVATE
    BARE_MEMORY_LIMIT=${BARE_MEMORY_LIMIT}
    BARE_DYNAMIC_ADDONS=$<BOOL:${BARE_DYNAMIC_ADDONS}>
    BARE_FILE_SYSTEM_MODULES=$<BOOL:${BARE_FILE_SYSTEM_MODULES}>
)

target_include_directories(
  bare_headers
  INTERFACE
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
)

link_bare_modules(bare::<TYPE> FROM src/builtins.json)

if(BARE_CLI)
  add_bare_bundle(
    bare_executable_bundle
    ENTRY bin/bare.js
    OUT bin/bare.bundle.h
    BUILTINS bin/builtins.json
    COMPRESSED
  )

  set(sources
    bin/bare.c
    bin/bare.bundle.h
  )

  if(target MATCHES "win32")
    list(APPEND sources bin/bare.manifest)
  endif()

  add_executable_target(
    bare
    SOURCES
      ${sources}
    LINK
      $<LINK_LIBRARY:WHOLE_ARCHIVE,bare_static>
      rlimit::static
    EXPORTS
      ${js_exports}
      ${napi_exports}
    EXPORTS_FROM
      src/exports.json
    OUTPUT_DIRECTORY
      bin
    EXPORT OFF
  )

  link_bare_modules(bare::executable FROM bin/builtins.json EXCLUDE_FROM src/builtins.json)
endif()

if(target MATCHES "win32")
  foreach(image IN ITEMS bare_shared bare_executable)
    if(TARGET ${image})
      target_link_libraries(
        ${image}
        PRIVATE
          delayimp
      )

      target_link_options(
        ${image}
        PRIVATE
          /DELAYLOAD:user32.dll
          /DELAYLOAD:shell32.dll
          /DELAYLOAD:ole32.dll
      )
    endif()
  endforeach()
endif()

add_test_subdirectory(test)
