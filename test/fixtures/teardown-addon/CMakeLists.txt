cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)

project(bare_teardown_addon C)

add_bare_module(teardown_addon)

target_sources(
  ${teardown_addon}
  PRIVATE
    binding.c
)
