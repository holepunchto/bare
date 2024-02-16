cmake_minimum_required(VERSION 3.25)

project(bare_addon C)

include(bare)

add_bare_bundle(
  ENTRY mod.js
  OUT mod.bundle
)

add_custom_target(
  bundle
  ALL
  DEPENDS mod.bundle
)
