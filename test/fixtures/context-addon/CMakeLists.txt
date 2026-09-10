cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)

project(bare_context_addon C)

add_bare_module(context_addon)

target_sources(
  ${context_addon}
  PRIVATE
    binding.c
)
