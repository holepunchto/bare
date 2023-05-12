cmake_minimum_required(VERSION 3.25)

project(bare_addon C)

include(bare)

add_bare_module(bare_addon)

target_sources(
  bare_addon
  PRIVATE
    binding.c
)
