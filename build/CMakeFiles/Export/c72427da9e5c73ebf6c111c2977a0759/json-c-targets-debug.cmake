#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "json-c::json-c" for configuration "Debug"
set_property(TARGET json-c::json-c APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(json-c::json-c PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libjson-c.5.3.0.dylib"
  IMPORTED_SONAME_DEBUG "@rpath/libjson-c.5.dylib"
  )

list(APPEND _cmake_import_check_targets json-c::json-c )
list(APPEND _cmake_import_check_files_for_json-c::json-c "${_IMPORT_PREFIX}/lib/libjson-c.5.3.0.dylib" )

# Import target "json-c::json-c-static" for configuration "Debug"
set_property(TARGET json-c::json-c-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(json-c::json-c-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libjson-c.a"
  )

list(APPEND _cmake_import_check_targets json-c::json-c-static )
list(APPEND _cmake_import_check_files_for_json-c::json-c-static "${_IMPORT_PREFIX}/lib/libjson-c.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
