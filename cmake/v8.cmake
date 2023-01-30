if(DEFINED V8)
  return()
endif()

set(V8_ROOT ${PROJECT_SOURCE_DIR}/prebuilds/v8/host)

set(V8 ${V8_ROOT}/lib/libv8.a)

set(V8_INCLUDE_DIR ${V8_ROOT}/include)
