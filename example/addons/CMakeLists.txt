cmake_minimum_required(VERSION 3.22)

project(add_numbers C CXX)

set(LIBRARY_OUTPUT_NAME, )

include_directories(
  add_numbers
  PUBLIC
    ../../include
    ../../deps/libjs/include
    ../../deps/libjs/vendor/libuv/include
)

add_library(
  add_numbers
  MODULE
    add.c
)

if(APPLE)
  set_target_properties(add_numbers PROPERTIES LINK_FLAGS "-undefined dynamic_lookup -Xlinker -no_fixup_chains")
endif(APPLE)

set_target_properties(add_numbers PROPERTIES PREFIX "")
set_target_properties(add_numbers PROPERTIES SUFFIX ".pear")
