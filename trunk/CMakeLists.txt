cmake_minimum_required(VERSION 2.4.0)

SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 OLD)
ENDIF()

PROJECT(mpmp)

IF(MSVC)
	# NOTE: Because we need to link to GTK which compiled with the Dynamic Runtime (/MD option),
	#       we cannot (easily?) support static linking.
	# TODO: See if we can't do something to link statically with the dynamic runtime?
	
	IF(TREAT_WARNINGS_AS_ERRORS)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
	ENDIF()

	# Extra speed optimisation options
	# NOTE: These *ARE* passed to clxx and friends, even if you can't tell by looking at the
	#       CMAKE_CXX_FLAGS in cmake-gui or properties window in Visual Studio
	SET(MSVC_EXTRA_OPTIMIZE ON CACHE BOOL "Use extra optimization flags in release builds")
	IF(MSVC_EXTRA_OPTIMIZE)
		SET(CMAKE_C_FLAGS_RELEASE             "${CMAKE_C_FLAGS_RELEASE}             /Ox /Ob2 /Oi /Ot /Oy /GT /GL /GF /FD /GS- /fp:fast")
		SET(CMAKE_CXX_FLAGS_RELEASE           "${CMAKE_CXX_FLAGS_RELEASE}           /Ox /Ob2 /Oi /Ot /Oy /GT /GL /GF /FD /GS- /fp:fast")
		SET(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    /OPT:REF /OPT:ICF /LTCG")
		SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /LTCG")
	ENDIF()
	
	# Even more speed optimisations
	SET(MSVC_OPTIMIZE_SSE2 OFF CACHE BOOL "Use SSE2 instruction set in release builds")
	IF(MSVC_OPTIMIZE_SSE2)
		SET(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /arch:SSE2")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE2")
	ENDIF()	
ENDIF()

IF(UNIX)
	ADD_DEFINITIONS("-D_LARGEFILE_SOURCE")
	ADD_DEFINITIONS("-D_FILE_OFFSET_BITS=64")
ENDIF()

IF(WIN32)
	ADD_DEFINITIONS("-DNOMINMAX")
	ADD_DEFINITIONS("-D_SCL_SECURE_NO_WARNINGS")
ENDIF()


IF(CMAKE_COMPILER_IS_GNUCXX)
	# Treat warnings as errors cmake option
	SET(TREAT_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors") # TODO: Support this for MINGW && MSVC also
	
	IF(TREAT_WARNINGS_AS_ERRORS)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
	ENDIF()
ENDIF()

IF(MINGW)
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -mwindows")
ENDIF()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

add_definitions(-DNETWORK_CORE_USE_LOCKS) # TODO: Make this configurable?

# Define DEBUG when doing debug builds.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

SUBDIRS(src)
