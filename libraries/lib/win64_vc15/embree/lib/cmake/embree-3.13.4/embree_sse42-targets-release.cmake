#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embree_sse42" for configuration "Release"
set_property(TARGET embree_sse42 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embree_sse42 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/embree_sse42.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS embree_sse42 )
list(APPEND _IMPORT_CHECK_FILES_FOR_embree_sse42 "${_IMPORT_PREFIX}/lib/embree_sse42.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
