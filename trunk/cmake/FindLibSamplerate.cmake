find_package(ExtraLibDir)

FIND_LIB_IN_EXTRALIBS(LIBSAMPLERATE *samplerate* include lib)

SET(LIBSAMPLERATE_FOUND FALSE)
SET(LIBSAMPLERATE_DEBUG_FOUND FALSE)

Find_Path(LIBSAMPLERATE_INCLUDE_DIR
	samplerate.h
	/usr/include /usr/local/include
	${LIBSAMPLERATE_EXTRALIB_INCLUDE_PATHS}
)

Find_Library(LIBSAMPLERATE_LIBRARY
	libsamplerate
	/usr/lib /usr/local/lib
	${LIBSAMPLERATE_EXTRALIB_LIBRARY_PATHS}
)

Find_Library(LIBSAMPLERATE_DEBUG_LIBRARY
	libsamplerated
	/usr/lib /usr/local/lib
	${LIBSAMPLERATE_EXTRALIB_LIBRARY_PATHS}
)

IF(NOT MSVC) # Assume non-MSVC compilers don't care about different runtimes
	SET(LIBSAMPLERATE_DEBUG_LIBRARY ${LIBSAMPLERATE_LIBRARY})
ENDIF(NOT MSVC)

IF(LIBSAMPLERATE_INCLUDE_DIR AND LIBSAMPLERATE_LIBRARY AND LIBSAMPLERATE_DEBUG_LIBRARY)
	SET(LIBSAMPLERATE_FOUND TRUE)
	SET(LIBSAMPLERATE_DEBUG_FOUND TRUE)
	MESSAGE(STATUS "Found samplerate library (libsamplerate)")
ENDIF(LIBSAMPLERATE_INCLUDE_DIR AND LIBSAMPLERATE_LIBRARY AND LIBSAMPLERATE_DEBUG_LIBRARY)
