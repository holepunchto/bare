set(V8 ${PROJECT_SOURCE_DIR}/prebuilds/v8/host)

add_library(v8 STATIC IMPORTED)

set_target_properties(
  v8
  PROPERTIES
  IMPORTED_LOCATION ${V8}/lib/libv8.a
)

target_include_directories(
  v8
  INTERFACE
    ${V8}/include
)
