# Generated by CMake

if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)
   message(FATAL_ERROR "CMake >= 2.8.0 required")
endif()
if(CMAKE_VERSION VERSION_LESS "2.8.3")
   message(FATAL_ERROR "CMake >= 2.8.3 required")
endif()
cmake_policy(PUSH)
cmake_policy(VERSION 2.8.3...3.22)
#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Protect against multiple inclusion, which would fail when already imported targets are added once more.
set(_cmake_targets_defined "")
set(_cmake_targets_not_defined "")
set(_cmake_expected_targets "")
foreach(_cmake_expected_target IN ITEMS OpenImageIO::OpenImageIO_Util OpenImageIO::OpenImageIO OpenImageIO::iconvert OpenImageIO::idiff OpenImageIO::igrep OpenImageIO::iinfo OpenImageIO::maketx OpenImageIO::oiiotool)
  list(APPEND _cmake_expected_targets "${_cmake_expected_target}")
  if(TARGET "${_cmake_expected_target}")
    list(APPEND _cmake_targets_defined "${_cmake_expected_target}")
  else()
    list(APPEND _cmake_targets_not_defined "${_cmake_expected_target}")
  endif()
endforeach()
unset(_cmake_expected_target)
if(_cmake_targets_defined STREQUAL _cmake_expected_targets)
  unset(_cmake_targets_defined)
  unset(_cmake_targets_not_defined)
  unset(_cmake_expected_targets)
  unset(CMAKE_IMPORT_FILE_VERSION)
  cmake_policy(POP)
  return()
endif()
if(NOT _cmake_targets_defined STREQUAL "")
  string(REPLACE ";" ", " _cmake_targets_defined_text "${_cmake_targets_defined}")
  string(REPLACE ";" ", " _cmake_targets_not_defined_text "${_cmake_targets_not_defined}")
  message(FATAL_ERROR "Some (but not all) targets in this export set were already defined.\nTargets Defined: ${_cmake_targets_defined_text}\nTargets not yet defined: ${_cmake_targets_not_defined_text}\n")
endif()
unset(_cmake_targets_defined)
unset(_cmake_targets_not_defined)
unset(_cmake_expected_targets)


# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

# Create imported target OpenImageIO::OpenImageIO_Util
add_library(OpenImageIO::OpenImageIO_Util STATIC IMPORTED)

set_target_properties(OpenImageIO::OpenImageIO_Util PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "OIIO_STATIC_DEFINE=1"
  INTERFACE_COMPILE_FEATURES "cxx_std_14"
  INTERFACE_INCLUDE_DIRECTORIES "C:/db/build/S/VS1564R/Release/openimageio/include;C:/db/build/S/VS1564R/Release/imath/include;C:/db/build/S/VS1564R/Release/openexr/include"
  INTERFACE_LINK_LIBRARIES "\$<TARGET_NAME_IF_EXISTS:Imath::Imath>;\$<TARGET_NAME_IF_EXISTS:Imath::Half>;\$<TARGET_NAME_IF_EXISTS:IlmBase::Imath>;\$<TARGET_NAME_IF_EXISTS:IlmBase::Half>;\$<TARGET_NAME_IF_EXISTS:IlmBase::IlmThread>;\$<TARGET_NAME_IF_EXISTS:IlmBase::Iex>;Imath::ImathConfig;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_filesystem-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_filesystem-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_system-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_system-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_thread-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_thread-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_chrono-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_chrono-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_atomic-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_atomic-vc142-mt-gd-x64-1_78.lib>;\$<LINK_ONLY:psapi>"
)

# Create imported target OpenImageIO::OpenImageIO
add_library(OpenImageIO::OpenImageIO STATIC IMPORTED)

set_target_properties(OpenImageIO::OpenImageIO PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "OIIO_STATIC_DEFINE=1"
  INTERFACE_COMPILE_FEATURES "cxx_std_14"
  INTERFACE_INCLUDE_DIRECTORIES "C:/db/build/S/VS1564R/Release/openimageio/include;C:/db/build/S/VS1564R/Release/imath/include;C:/db/build/S/VS1564R/Release/openexr/include"
  INTERFACE_LINK_LIBRARIES "OpenImageIO::OpenImageIO_Util;\$<TARGET_NAME_IF_EXISTS:Imath::Imath>;\$<TARGET_NAME_IF_EXISTS:Imath::Half>;\$<TARGET_NAME_IF_EXISTS:IlmBase::Imath>;\$<TARGET_NAME_IF_EXISTS:IlmBase::Half>;Imath::ImathConfig;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:OpenEXR::OpenEXR>>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:OpenEXR::IlmImf>>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:IlmBase::IlmThread>>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:IlmBase::Iex>>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:Imath::Imath>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::Iex>;\$<LINK_ONLY:OpenEXR::IlmThread>;\$<LINK_ONLY:ZLIB::ZLIB>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::Iex>;\$<LINK_ONLY:Threads::Threads>;\$<LINK_ONLY:Imath::ImathConfig>;\$<LINK_ONLY:PNG::PNG>;\$<LINK_ONLY:ZLIB::ZLIB>;C:/db/build/S/VS1564R/Release/jpeg/lib/jpeg-static.lib;C:/db/build/S/VS1564R/Release/openjpeg_msvc/lib/openjp2.lib;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:Imath::Imath>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::Iex>;\$<LINK_ONLY:OpenEXR::IlmThread>;\$<LINK_ONLY:ZLIB::ZLIB>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::IlmThreadConfig>;\$<LINK_ONLY:OpenEXR::Iex>;\$<LINK_ONLY:Threads::Threads>;\$<LINK_ONLY:Imath::ImathConfig>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:OpenEXR::OpenEXRCore>>;\$<LINK_ONLY:PNG::PNG>;\$<LINK_ONLY:ZLIB::ZLIB>;C:/db/build/S/VS1564R/Release/jpeg/lib/jpeg-static.lib;C:/db/build/S/VS1564R/Release/tiff/lib/tiff.lib;C:/db/build/S/VS1564R/Release/jpeg/lib/jpeg-static.lib;\$<LINK_ONLY:ZLIB::ZLIB>;C:/db/build/S/VS1564R/Release/webp/lib/webp.lib;C:/db/build/S/VS1564R/Release/webp/lib/webpdemux.lib;\$<LINK_ONLY:ZLIB::ZLIB>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:OpenColorIO::OpenColorIO>>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:OpenColorIO::OpenColorIOHeaders>>;\$<LINK_ONLY:\$<TARGET_NAME_IF_EXISTS:pugixml::pugixml>>;\$<LINK_ONLY:ZLIB::ZLIB>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_filesystem-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_filesystem-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_system-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_system-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_thread-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_thread-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_chrono-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_chrono-vc142-mt-gd-x64-1_78.lib>;\$<\$<NOT:\$<CONFIG:DEBUG>>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_atomic-vc142-mt-x64-1_78.lib>;\$<\$<CONFIG:DEBUG>:C:/db/build/S/VS1564R/Release/boost/lib/libboost_atomic-vc142-mt-gd-x64-1_78.lib>;\$<LINK_ONLY:psapi>"
)

# Create imported target OpenImageIO::iconvert
add_executable(OpenImageIO::iconvert IMPORTED)

# Create imported target OpenImageIO::idiff
add_executable(OpenImageIO::idiff IMPORTED)

# Create imported target OpenImageIO::igrep
add_executable(OpenImageIO::igrep IMPORTED)

# Create imported target OpenImageIO::iinfo
add_executable(OpenImageIO::iinfo IMPORTED)

# Create imported target OpenImageIO::maketx
add_executable(OpenImageIO::maketx IMPORTED)

# Create imported target OpenImageIO::oiiotool
add_executable(OpenImageIO::oiiotool IMPORTED)

if(CMAKE_VERSION VERSION_LESS 2.8.12)
  message(FATAL_ERROR "This file relies on consumers using CMake 2.8.12 or greater.")
endif()

# Load information for each installed configuration.
file(GLOB _cmake_config_files "${CMAKE_CURRENT_LIST_DIR}/OpenImageIOTargets-*.cmake")
foreach(_cmake_config_file IN LISTS _cmake_config_files)
  include("${_cmake_config_file}")
endforeach()
unset(_cmake_config_file)
unset(_cmake_config_files)

# Cleanup temporary variables.
set(_IMPORT_PREFIX)

# Loop over all imported files and verify that they actually exist
foreach(_cmake_target IN LISTS _cmake_import_check_targets)
  foreach(_cmake_file IN LISTS "_cmake_import_check_files_for_${_cmake_target}")
    if(NOT EXISTS "${_cmake_file}")
      message(FATAL_ERROR "The imported target \"${_cmake_target}\" references the file
   \"${_cmake_file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    endif()
  endforeach()
  unset(_cmake_file)
  unset("_cmake_import_check_files_for_${_cmake_target}")
endforeach()
unset(_cmake_target)
unset(_cmake_import_check_targets)

# This file does not depend on other imported targets which have
# been exported from the same project but in a separate export set.

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
cmake_policy(POP)
