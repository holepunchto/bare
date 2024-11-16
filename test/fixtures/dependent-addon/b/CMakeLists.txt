cmake_minimum_required(VERSION 3.25)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)

project(bare_addon_b C)

include_bare_module(../a addon_a)

add_bare_module(addon_b)

target_sources(
  ${addon_b}
  PRIVATE
    binding.c
)

target_link_libraries(
  ${addon_b}
  PUBLIC
    ${addon_a}_module
)
