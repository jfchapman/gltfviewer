
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was AlembicConfig.cmake.in                            ########

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
# TODO whenever we loose the back-compatibility with IlmBase < 3, a REQUIRED needs to be added to find_dependency()
if (ON)
    find_dependency(Imath)
else ()
    # Compatibility with OpenEXR 2.x, prior to Imath 3.x
    find_dependency(IlmBase)
endif ()

SET(Alembic_HAS_HDF5 OFF)
SET(Alembic_HAS_SHARED_LIBS OFF)

INCLUDE("${CMAKE_CURRENT_LIST_DIR}/AlembicTargets.cmake")
check_required_components("Alembic")
