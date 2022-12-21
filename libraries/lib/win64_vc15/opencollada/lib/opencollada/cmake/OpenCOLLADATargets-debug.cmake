#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pcre_static" for configuration "Debug"
set_property(TARGET pcre_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(pcre_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/pcre_d.lib"
  )

list(APPEND _cmake_import_check_targets pcre_static )
list(APPEND _cmake_import_check_files_for_pcre_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/pcre_d.lib" )

# Import target "ftoa_static" for configuration "Debug"
set_property(TARGET ftoa_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(ftoa_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/ftoa_d.lib"
  )

list(APPEND _cmake_import_check_targets ftoa_static )
list(APPEND _cmake_import_check_files_for_ftoa_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/ftoa_d.lib" )

# Import target "UTF_static" for configuration "Debug"
set_property(TARGET UTF_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(UTF_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/UTF_d.lib"
  )

list(APPEND _cmake_import_check_targets UTF_static )
list(APPEND _cmake_import_check_files_for_UTF_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/UTF_d.lib" )

# Import target "buffer_static" for configuration "Debug"
set_property(TARGET buffer_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(buffer_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "ftoa_static;UTF_static"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/buffer_d.lib"
  )

list(APPEND _cmake_import_check_targets buffer_static )
list(APPEND _cmake_import_check_files_for_buffer_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/buffer_d.lib" )

# Import target "MathMLSolver_static" for configuration "Debug"
set_property(TARGET MathMLSolver_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MathMLSolver_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/MathMLSolver_d.lib"
  )

list(APPEND _cmake_import_check_targets MathMLSolver_static )
list(APPEND _cmake_import_check_files_for_MathMLSolver_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/MathMLSolver_d.lib" )

# Import target "OpenCOLLADABaseUtils_static" for configuration "Debug"
set_property(TARGET OpenCOLLADABaseUtils_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenCOLLADABaseUtils_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "UTF_static;pcre_static"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADABaseUtils_d.lib"
  )

list(APPEND _cmake_import_check_targets OpenCOLLADABaseUtils_static )
list(APPEND _cmake_import_check_files_for_OpenCOLLADABaseUtils_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADABaseUtils_d.lib" )

# Import target "OpenCOLLADAFramework_static" for configuration "Debug"
set_property(TARGET OpenCOLLADAFramework_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenCOLLADAFramework_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "OpenCOLLADABaseUtils_static;MathMLSolver_static"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADAFramework_d.lib"
  )

list(APPEND _cmake_import_check_targets OpenCOLLADAFramework_static )
list(APPEND _cmake_import_check_files_for_OpenCOLLADAFramework_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADAFramework_d.lib" )

# Import target "GeneratedSaxParser_static" for configuration "Debug"
set_property(TARGET GeneratedSaxParser_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(GeneratedSaxParser_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "OpenCOLLADABaseUtils_static;C:/db/build/S/VS1564D/Debug/xml2/lib/libxml2sd.lib"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/GeneratedSaxParser_d.lib"
  )

list(APPEND _cmake_import_check_targets GeneratedSaxParser_static )
list(APPEND _cmake_import_check_files_for_GeneratedSaxParser_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/GeneratedSaxParser_d.lib" )

# Import target "OpenCOLLADASaxFrameworkLoader_static" for configuration "Debug"
set_property(TARGET OpenCOLLADASaxFrameworkLoader_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenCOLLADASaxFrameworkLoader_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "OpenCOLLADABaseUtils_static;GeneratedSaxParser_static;OpenCOLLADAFramework_static;MathMLSolver_static;pcre_static;C:/db/build/S/VS1564D/Debug/xml2/lib/libxml2sd.lib"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADASaxFrameworkLoader_d.lib"
  )

list(APPEND _cmake_import_check_targets OpenCOLLADASaxFrameworkLoader_static )
list(APPEND _cmake_import_check_files_for_OpenCOLLADASaxFrameworkLoader_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADASaxFrameworkLoader_d.lib" )

# Import target "OpenCOLLADAStreamWriter_static" for configuration "Debug"
set_property(TARGET OpenCOLLADAStreamWriter_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenCOLLADAStreamWriter_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "OpenCOLLADABaseUtils_static;buffer_static;ftoa_static"
  IMPORTED_LOCATION_DEBUG "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADAStreamWriter_d.lib"
  )

list(APPEND _cmake_import_check_targets OpenCOLLADAStreamWriter_static )
list(APPEND _cmake_import_check_files_for_OpenCOLLADAStreamWriter_static "C:/db/build/S/VS1564D/Debug/opencollada/lib/opencollada/OpenCOLLADAStreamWriter_d.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
