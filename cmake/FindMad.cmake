find_package(ExtraLibDir)

FIND_LIB_IN_EXTRALIBS(LIBMAD *mad* include lib)

Find_Path(MAD_INCLUDE_DIR
	mad.h
	usr/include usr/local/include
	"C:/Program Files/libmad-0.15.1b/"
	"D:/libmad-0.15.1b/"
	${LIBMAD_EXTRALIB_INCLUDE_PATHS}
)

Find_Library(MAD_LIBRARY
	mad
	usr/lib usr/local/lib
	"C:/Program Files/libmad-0.15.1b/.libs"
	"D:/libmad-0.15.1b/.libs"
	${LIBMAD_EXTRALIB_LIBRARY_PATHS}
)

SET(MAD_FOUND FALSE)
IF (MAD_INCLUDE_DIR AND MAD_LIBRARY)
	SET(MAD_FOUND TRUE)
	MESSAGE(STATUS "Found Mad library")
ENDIF (MAD_INCLUDE_DIR AND MAD_LIBRARY)
