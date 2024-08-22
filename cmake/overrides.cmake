function(add_bare_module result)
  bare_module_target("." target NAME name)

  add_library(${target} OBJECT)

  set_target_properties(
    ${target}
    PROPERTIES
    C_STANDARD 11
    CXX_STANDARD 20
    POSITION_INDEPENDENT_CODE ON
  )

  target_include_directories(
    ${target}
    PRIVATE
      $<TARGET_PROPERTY:bare,INTERFACE_INCLUDE_DIRECTORIES>
  )

  set(${result} ${target})

  return(PROPAGATE ${result})
endfunction()
