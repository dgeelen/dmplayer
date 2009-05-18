cmake_minimum_required(VERSION 2.4.0)

IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

PROJECT(mpmp)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Use DEBUG as default build type for now
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# Treat warnings as errors cmake option (todo: support this on gcc/other compilers)
SET(TREAT_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors")

# Treat warnings as errors cmake option
SET(TREAT_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors")

IF(MSVC)
	SET(LINK_STATIC_RUNTIME OFF CACHE BOOL "Link statically against C++ runtime")
	ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")

	IF(TREAT_WARNINGS_AS_ERRORS)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
	ENDIF()

	# Extra speed optimisation options
	SET(MSVC_EXTRA_OPTIMIZE ON CACHE BOOL "Use extra optimization flags in release builds")
	IF(MSVC_EXTRA_OPTIMIZE)
		SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oi /Ot /Oy /GL /GS- /fp:fast")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oi /Ot /Oy /GL /GS- /fp:fast")
		SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /LTCG")
		SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /LTCG")
	ENDIF()

	#SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:LIBCMTD")
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:LIBCMT /NODEFAULTLIB:libcpmt")

	IF(LINK_STATIC_RUNTIME)
		foreach(cfgtype ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
			string(TOUPPER "CMAKE_C_FLAGS_${cfgtype}" flag_var)
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			string(TOUPPER "CMAKE_CXX_FLAGS_${cfgtype}" flag_var)
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endforeach()
		ADD_DEFINITIONS("-DLINK_STATIC_RUNTIME")
		#SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:msvcrt /NODEFAULTLIB:msvcrtd")
	ENDIF()
ENDIF()

# Define DEBUG when doing debug builds.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
add_definitions(-DNETWORK_CORE_USE_LOCKS)

SUBDIRS(src)
