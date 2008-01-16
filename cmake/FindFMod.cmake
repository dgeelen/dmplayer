# - Find boost libraries
#

# This module defines
#  FMOD_INCLUDE_DIR, location of FMod headers
#  FMOD_LIBRARY_DIR, location of FMod libraries

FIND_PATH(FMOD_INCLUDE_DIR
	NAMES fmod.hpp
	PATHS
		/usr/include/fmodex /usr/local/include/fmodex
		"D:/FMod Programmers API Win32/api/inc"
		"C:/Program Files/FMod Programmers API Win32/api/inc"     
)

FIND_PATH(FMOD_LIBRARY_DIR
	NAMES libfmodexp.a
	PATHS
		/usr/lib /usr/local/lib
		"D:/FMod Programmers API Win32/api/lib"
		"C:/Program Files/FMod Programmers API Win32/api/lib"     
)

FIND_LIBRARY(FMOD_LIBRARY
	fmodexp
	PATHS "${FMOD_LIBRARY_DIR}" 
	      /usr/lib /usr/local/lib
)

MARK_AS_ADVANCED(FMOD_LIBRARY_DIR)

