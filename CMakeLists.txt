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

IF(MSVC)
	# Prevent link warning in msvc
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD,LIBCMT")

	IF(TREAT_WARNINGS_AS_ERRORS)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
	ENDIF(TREAT_WARNINGS_AS_ERRORS)
ENDIF(MSVC)

# Define DEBUG when doing debug builds.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
add_definitions(-DNETWORK_CORE_USE_LOCKS)
SUBDIRS(src)
