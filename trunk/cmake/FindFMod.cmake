# - Find boost libraries
#

# This module defines
#  FMOD_INCLUDE_DIR, location of boost headers
#  FMOD_LIBRARY_DIR, location of boost libraries


FIND_PATH(FMOD_INCLUDE_DIR
	NAMES FMod.h
	PATHS
		/usr/include /usr/local/include
		"${CMAKE_SOURCE_DIR}/src/audio/mp3"
)

IF(WIN32 AND NOT MINGW)
	# depend on autolinking on windows
	SET(BOOST_THREAD_LIBRARY "")
	SET(BOOST_FILESYSTEM_LIBRARY "")
	SET(BOOST_PROGRAM_OPTIONS_LIBRARY "")
	SET(BOOST_LIBRARY_DIR
		"${BOOST_INCLUDE_DIR}/lib"
		CACHE PATH
		"path to boost library files"
	)
ELSE(WIN32 AND NOT MINGW)
	SET(FMOD_LIBRARY_DIR
		"${FMOD_INCLUDE_DIR}"
	)

	FIND_LIBRARY(FMOD_LIBRARY
		fmod
		PATHS "${FMOD_LIBRARY_DIR}" 
		      /usr/lib /usr/local/lib
	)

ENDIF(WIN32 AND NOT MINGW)

MARK_AS_ADVANCED(FMOD_LIBRARY_DIR)

