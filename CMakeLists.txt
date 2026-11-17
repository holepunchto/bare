cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-harden REQUIRED PATHS node_modules/cmake-harden)
find_package(cmake-bare-bundle REQUIRED PATHS node_modules/cmake-bare-bundle)
find_package(cmake-drive REQUIRED PATHS node_modules/cmake-drive)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-napi REQUIRED PATHS node_modules/cmake-napi)
find_package(cmake-prelude REQUIRED PATHS node_modules/cmake-prelude)
find_package(cmake-test REQUIRED PATHS node_modules/cmake-test)

project(bare LANGUAGES C CXX VERSION 1.38.0)

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

set(BARE_ENGINE "github:holepunchto/libjs#0609f8e" CACHE STRING "The JavaScript engine to use")

set(BARE_PREBUILDS ON CACHE BOOL "Enable prebuilds for supported third-party dependencies")

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

if(BARE_PREBUILDS AND BARE_ENGINE MATCHES "github:holepunchto/libjs(#|@|$)")
  set(prebuilds "${CMAKE_CURRENT_BINARY_DIR}")

  mirror_drive(
    SOURCE krmguqnh75rud4iieqgmzyqoza415cbu68payoww65ii9fzt5n4o
    DESTINATION "${prebuilds}"
    PREFIX /${target}
    CHECKOUT 317
  )

  add_library(c++ STATIC IMPORTED GLOBAL)

  find_library(
    c++
    NAMES c++ libc++
    PATHS "${prebuilds}/${target}"
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
    target_compile_options(
      c++
      INTERFACE
        /GR-
    )

    set(debug_crt "$<STREQUAL:$<GENEX_EVAL:$<TARGET_PROPERTY:MSVC_RUNTIME_LIBRARY>>,MultiThreadedDebug>")

    target_link_options(
      c++
      INTERFACE
        /DEFAULTLIB:libcpmt$<${debug_crt}:d>
        /NODEFAULTLIB:libcmt$<$<NOT:${debug_crt}>:d>
    )
  else()
    target_compile_options(
      c++
      INTERFACE
        -fno-rtti
    )
  endif()

  add_library(v8 STATIC IMPORTED GLOBAL)

  find_library(
    v8
    NAMES v8 libv8
    PATHS "${prebuilds}/${target}"
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
  elseif(target MATCHES "darwin|ios")
    target_link_libraries(
      v8
      INTERFACE
        "-framework CoreFoundation"
    )
  elseif(target MATCHES "win32")
    target_link_libraries(
      v8
      INTERFACE
        winmm
    )
  endif()

  add_library(js STATIC IMPORTED GLOBAL)

  add_library(js::static ALIAS js)
  add_library(js::shared ALIAS js)

  find_library(
    js
    NAMES js libjs
    PATHS "${prebuilds}/${target}"
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

  add_library(js_headers INTERFACE)

  add_library(js::headers ALIAS js_headers)

  target_link_libraries(
    js
    INTERFACE
      js_headers
  )
endif()

fetch_package("${BARE_ENGINE}")

fetch_package("github:holepunchto/libnapi#81af589")

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
    ${CMAKE_CURRENT_BINARY_DIR}/include/bare/version.h
  BASE_DIRS
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
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
      $<LINK_LIBRARY:WHOLE_ARCHIVE,js::static>
    EMBED
      uv
      napi
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

if(TARGET bare_shared)
  set_target_properties(
    bare_shared
    PROPERTIES
    ENABLE_EXPORTS ON
  )

  if(target MATCHES "win32")
    add_library(bare_import_library INTERFACE)

    add_library(bare::import_library ALIAS bare_import_library)

    target_link_libraries(
      bare_import_library
      INTERFACE
        $<TARGET_LINKER_FILE:bare_shared>
    )

    add_dependencies(bare_import_library bare_shared)
  endif()
endif()

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
    EXPORTS_FROM
      src/exports.json
    OUTPUT_DIRECTORY
      bin
    EXPORT OFF
  )

  link_bare_modules(bare::executable FROM bin/builtins.json EXCLUDE_FROM src/builtins.json)

  # BoringSSL is compiled against the MSVC standard library while the prebuilt
  # libc++ defines some of the same symbols. Linking libc++ after BoringSSL lets
  # the copies in BoringSSL satisfy the references, so that the conflicting
  # objects of libc++ aren't pulled in.
  if(TARGET c++)
    target_link_libraries(ssl c++)
  endif()
endif()

add_test_subdirectory(test)
