find_package(SearchUtils)

STRING(COMPARE EQUAL "${CMAKE_EXTRA_LIBS_DIR}" "" AUTODETECT_LIB_DIR)
IF(AUTODETECT_LIB_DIR)
	GLOB_PATHS(CMAKE_EXTRA_LIBS_PATH_ROOT
		NOBASERESULT
		GLOBS   extralibs
		BASES
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/..
			${CMAKE_CURRENT_SOURCE_DIR}/../..
			${CMAKE_CURRENT_SOURCE_DIR}/../../..
			${CMAKE_CURRENT_SOURCE_DIR}/../../../..
			${CMAKE_CURRENT_SOURCE_DIR}/../../../../..
			${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..
	)
	SET(CMAKE_EXTRA_LIBS_PATH "${CMAKE_EXTRA_LIBS_PATH_ROOT}" CACHE PATH "Paths where to look for extra libraries")
ENDIF(AUTODETECT_LIB_DIR)

MACRO(FIND_LIB_IN_EXTRALIBS prefix mainglob includeglob libglob)
	SET(${prefix}_EXTRALIB_INCLUDE_PATHS)
	SET(${prefix}_EXTRALIB_LIBRARY_PATHS)
	GLOB_PATHS(${prefix}_EXTRALIB_BASE_DIRS
		NOBASERESULT
		GLOBS ${mainglob}
		BASES ${CMAKE_EXTRA_LIBS_PATH}
	)

	IF(${prefix}_EXTRALIB_BASE_DIRS)
		GLOB_PATHS(${prefix}_EXTRALIB_INCLUDE_PATHS
			NOBASERESULT
			GLOBS ${includeglob}
			BASES ${${prefix}_EXTRALIB_BASE_DIRS}
		)

		GLOB_PATHS(${prefix}_EXTRALIB_LIBRARY_PATHS
			NOBASERESULT
			GLOBS ${libglob}
			BASES ${${prefix}_EXTRALIB_BASE_DIRS}
		)
	ENDIF(${prefix}_EXTRALIB_BASE_DIRS)

ENDMACRO(FIND_LIB_IN_EXTRALIBS)
