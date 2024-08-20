function(download_bare result)
  cmake_parse_arguments(
    PARSE_ARGV 1 ARGV "" "IMPORT_FILE" ""
  )

  set(import_file ${ARGV_IMPORT_FILE})

  set(${result} $<TARGET_FILE:bare_bin>)

  if(import_file)
    set(${import_file} $<TARGET_IMPORT_FILE:bare_bin>)
  endif()

  return(PROPAGATE ${result} ${import_file})
endfunction()

function(download_bare_headers result)
  set(${result} $<TARGET_PROPERTY:bare,INTERFACE_INCLUDE_DIRECTORIES>)

  return(PROPAGATE ${result})
endfunction()
