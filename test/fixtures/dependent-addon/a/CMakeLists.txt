cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)

project(bare_addon_a C)

add_bare_module(addon_a EXPORTS)

target_sources(
  ${addon_a}
  PRIVATE
    binding.c
)
