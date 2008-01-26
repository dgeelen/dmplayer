PROJECT(mpmp)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Use DEBUG as default build type for now
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif (NOT CMAKE_BUILD_TYPE)

IF(MSVC)
	# Prevent link warning in msvc
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD")
	# Treat warnings as errors <TODO> make this a cmake option?
	# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
ENDIF(MSVC)

# Define DEBUG when doing debug builds.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG" CACHE STRING
  "Flags used by the compiler during debug builds." FORCE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG" CACHE STRING
  "Flags used by the compiler during debug builds." FORCE)

SUBDIRS(src)
