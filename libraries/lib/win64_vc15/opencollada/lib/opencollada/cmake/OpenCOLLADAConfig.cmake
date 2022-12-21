# - Configuration file for the OpenCOLLADA library
# It defines the following variables:
#  OPENCOLLADA_INCLUDE_DIRS - include directories for OpenCOLLADA
#  OPENCOLLADA_LIBRARIES - libraries to link against OpenCOLLADA
#  OPENCOLLADA_VERSION_MAJOR - major version of OpenCOLLADA
#  OPENCOLLADA_VERSION_MINOR - minor version of OpenCOLLADA
#  OPENCOLLADA_VERSION_PATCH - patch version of OpenCOLLADA
#  OPENCOLLADA_VERSION_STRING - human-readable string containing the version of OpenCOLLADA

# Set the version of OpenCOLLADA
set(OPENCOLLADA_VERSION_MAJOR 0)
set(OPENCOLLADA_VERSION_MINOR 1)
set(OPENCOLLADA_VERSION_PATCH 0)
set(OPENCOLLADA_VERSION_STRING ${OPENCOLLADA_VERSION_MAJOR}.${OPENCOLLADA_VERSION_MINOR}.${OPENCOLLADA_VERSION_PATCH})


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was OpenCOLLADAConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

####################################################################################

# Set the include directory
set(OPENCOLLADA_INCLUDE_DIRS
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADABaseUtils"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADABaseUtils/Math"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADAFramework"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADASaxFrameworkLoader"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADASaxFrameworkLoader/generated14"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADASaxFrameworkLoader/generated15"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/COLLADAStreamWriter"
	"${PACKAGE_PREFIX_DIR}/include/opencollada/GeneratedSaxParser"
)

# Import the exported targets
include("${PACKAGE_PREFIX_DIR}/lib/opencollada/cmake/OpenCOLLADATargets.cmake")

# Set the library variable
if(UNIX)
	set(OPENCOLLADA_LIBRARIES
		ftoa_shared
		buffer_shared
		UTF_shared
		MathMLSolver_shared
		OpenCOLLADABaseUtils_shared
		OpenCOLLADAFramework_shared
		GeneratedSaxParser_shared
		OpenCOLLADASaxFrameworkLoader_shared
		OpenCOLLADAStreamWriter_shared
	)
else()
	set(OPENCOLLADA_LIBRARIES
		ftoa_static
		buffer_static
		UTF_static
		MathMLSolver_static
		OpenCOLLADABaseUtils_static
		OpenCOLLADAFramework_static
		GeneratedSaxParser_static
		OpenCOLLADASaxFrameworkLoader_static
		OpenCOLLADAStreamWriter_static
	)
endif()
