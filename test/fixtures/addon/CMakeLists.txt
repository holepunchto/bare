cmake_minimum_required(VERSION 3.25)

project(pear_addon C)

include(pear)

add_pear_module(pear_addon)

target_sources(
  pear_addon
  PRIVATE
    binding.c
)
