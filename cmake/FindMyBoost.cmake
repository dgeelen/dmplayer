# - Find boost libraries
#

# This module defines
#  BOOST_INCLUDE_DIR, location of boost headers
#  BOOST_LIBRARY_DIR, location of boost libraries
#  BOOST_XXX_LIBRARY, the library to link against BOOST_XXX lib

# also defined, but not for general use are
#  BOOST_LIBRARY_SUFFIX, suffix to try for finding the boost libraries (unix only)

find_package(SearchUtils)

GLOB_PATHS(BOOST_SEARCH_PATHS
	GLOBS "boost_[0-9_]*"
	BASES	
		/usr/include /usr/local/include
		"C:/Program Files/boost"
		"C:/Program Files"
		"C:/Boost"
		"D:/Prog/Boost"
		
)

FIND_PATH(BOOST_INCLUDE_DIR
	NAMES boost/config.hpp
	PATHS ${BOOST_SEARCH_PATHS}
)

IF(WIN32 AND NOT MINGW)
	# depend on autolinking on windows
	SET(BOOST_THREAD_LIBRARY "")
	SET(BOOST_PROGRAM_OPTIONS_LIBRARY "")
	SET(BOOST_FILESYSTEM_LIBRARY "")
	SET(BOOST_SIGNALS_LIBRARY "")
	SET(BOOST_LIBRARY_DIR
		"${BOOST_INCLUDE_DIR}/lib"
		CACHE PATH
		"path to boost library files"
	)
ELSE(WIN32 AND NOT MINGW)
	SET(BOOST_LIBRARY_DIR
		"${BOOST_INCLUDE_DIR}/../lib"
		CACHE PATH
		"path to boost library files"
	)

	IF (NOT BOOST_LIBRARY_SUFFIX)
		MESSAGE(STATUS "No BOOST_LIBRARY_SUFFIX")
		# TODO: autodetect BOOST_LIBRARY_SUFFIX somehow
	ENDIF (NOT BOOST_LIBRARY_SUFFIX)

	SET(BOOST_LIBRARY_SUFFIX
		-gcc41-mt
		CACHE STRING
		"suffix to use for boost libraries"
	)

	FIND_LIBRARY(BOOST_THREAD_LIBRARY
		NAMES boost_thread-mt boost_thread${BOOST_LIBRARY_SUFFIX}
		PATHS "${BOOST_LIBRARY_DIR}"
		      "${BOOST_INCLUDE_DIR}/lib"
		      "${BOOST_INCLUDE_DIR}/../lib"
		      /usr/lib /usr/local/lib
	)

	FIND_LIBRARY(BOOST_PROGRAM_OPTIONS_LIBRARY
		NAMES boost_program_options-mt boost_program_options${BOOST_LIBRARY_SUFFIX}
		PATHS "${BOOST_LIBRARY_DIR}"
		      "${BOOST_INCLUDE_DIR}/lib"
		      "${BOOST_INCLUDE_DIR}/../lib"
		      /usr/lib /usr/local/lib
	)

	FIND_LIBRARY(BOOST_FILESYSTEM_LIBRARY
		NAMES boost_filesystem-mt boost_filesystem${BOOST_LIBRARY_SUFFIX}
		PATHS "${BOOST_LIBRARY_DIR}"
		      "${BOOST_INCLUDE_DIR}/lib"
		      "${BOOST_INCLUDE_DIR}/../lib"
		      /usr/lib /usr/local/lib
	)

	FIND_LIBRARY(BOOST_SIGNALS_LIBRARY
		NAMES boost_signals-mt boost_signals${BOOST_LIBRARY_SUFFIX}
		PATHS "${BOOST_LIBRARY_DIR}"
		      "${BOOST_INCLUDE_DIR}/lib"
		      "${BOOST_INCLUDE_DIR}/../lib"
		      /usr/lib /usr/local/lib
	)
ENDIF(WIN32 AND NOT MINGW)

MARK_AS_ADVANCED(BOOST_LIBRARY_DIR)
