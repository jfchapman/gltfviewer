# Copyright Contributors to the Open Shading Language project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/AcademySoftwareFoundation/OpenShadingLanguage


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# add here all the find_dependency() whenever switching to config based dependencies
if (3.1.5 VERSION_GREATER_EQUAL 3.0)
    find_dependency(Imath 3.1.5)
elseif (3.1.5 VERSION_GREATER_EQUAL 2.4 AND 1)
    find_dependency(IlmBase 3.1.5)
    find_dependency(OpenEXR 3.1.5)
    find_dependency(ZLIB 1.2.13)  # Because OpenEXR doesn't do it
    find_dependency(Threads)  # Because OpenEXR doesn't do it
endif ()


find_dependency(OpenImageIO 2.3.20.0 REQUIRED)

# Compute the installation prefix relative to this file
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# There's no guarantee that CMAKE_INSTALL_XXXDIR is a relative path so we extract the final component to handle absolute paths
get_filename_component(_INCLUDE_DIR_NAME "include" NAME)
get_filename_component(_LIB_DIR_NAME "lib" NAME)

set_and_check (OSL_INCLUDE_DIR "${_IMPORT_PREFIX}/${_INCLUDE_DIR_NAME}")
set_and_check (OSL_INCLUDES    "${_IMPORT_PREFIX}/${_INCLUDE_DIR_NAME}")
set_and_check (OSL_LIB_DIR     "${_IMPORT_PREFIX}/${_LIB_DIR_NAME}")

#...logic to determine installedPrefix from the own location...
#set (OSL_CONFIG_DIR  "${installedPrefix}/")

include ("${CMAKE_CURRENT_LIST_DIR}/OSLTargets.cmake")

check_required_components ("OSL")
