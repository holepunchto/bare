cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)

project(bare_constructor_addon C)

add_bare_module(constructor_addon)

target_compile_definitions(
  ${constructor_addon}
  PRIVATE
    BARE_MODULE_REGISTER_CONSTRUCTOR
    BARE_MODULE_CONSTRUCTOR_VERSION=0
)

target_sources(
  ${constructor_addon}
  PRIVATE
    binding.c
)
