cmake_minimum_required(VERSION 3.25)

project(bare_addon C)

include(bare)

add_bare_module(addon)

target_sources(
  ${addon}
  PRIVATE
    binding.c
)
