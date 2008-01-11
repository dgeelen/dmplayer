# - Find boost libraries
#

# This module defines
#  FMOD_INCLUDE_DIR, location of boost headers
#  FMOD_LIBRARY_DIR, location of boost libraries


FIND_PATH(FMOD_INCLUDE_DIR
	NAMES fmod.h
	PATHS
		/usr/include /usr/local/include
		"${CMAKE_SOURCE_DIR}/src/audio/mp3"
)

SET(FMOD_LIBRARY_DIR
	"${FMOD_INCLUDE_DIR}"
)

FIND_LIBRARY(FMOD_LIBRARY
	fmod
	PATHS "${FMOD_LIBRARY_DIR}" 
	      /usr/lib /usr/local/lib
)

MARK_AS_ADVANCED(FMOD_LIBRARY_DIR)

