find_package(ExtraLibDir)

FIND_LIB_IN_EXTRALIBS(LIBMAD *mad* include lib)

Find_Path(LIBMAD_INCLUDE_DIR
	mad.h
	usr/include usr/local/include
	"C:/Program Files/libmad-0.15.1b/"
	"D:/libmad-0.15.1b/"
	${LIBMAD_EXTRALIB_INCLUDE_PATHS}
)

Find_Library(LIBMAD_LIBRARY
	mad
	usr/lib usr/local/lib
	"C:/Program Files/libmad-0.15.1b/.libs"
	"D:/libmad-0.15.1b/.libs"
	${LIBMAD_EXTRALIB_LIBRARY_PATHS}
)

Find_Library(LIBMAD_DEBUG_LIBRARY
	madd
	usr/lib usr/local/lib
	"C:/Program Files/libmad-0.15.1b/.libs"
	"D:/libmad-0.15.1b/.libs"
	${LIBMAD_EXTRALIB_LIBRARY_PATHS}
)

IF(NOT MSVC) # Assume non-MSVC compilers don't care about different runtimes
	SET(LIBMAD_DEBUG_LIBRARY ${LIBMAD_LIBRARY})
ENDIF(NOT MSVC)

SET(LIBMAD_FOUND FALSE)
SET(LIBMAD_DEBUG_FOUND FALSE)
IF(LIBMAD_INCLUDE_DIR AND LIBMAD_LIBRARY AND LIBMAD_DEBUG_LIBRARY)
	SET(LIBMAD_FOUND TRUE)
	SET(LIBMAD_DEBUG_FOUND TRUE)
	MESSAGE(STATUS "Found Mad library")
ENDIF(LIBMAD_INCLUDE_DIR AND LIBMAD_LIBRARY AND LIBMAD_DEBUG_LIBRARY)
