find_package(ExtraLibDir)

FIND_LIB_IN_EXTRALIBS(LIBVORBIS *vorbis* include lib)

SET(LIBVORBIS_FOUND FALSE)
SET(LIBVORBISFILE_FOUND FALSE)

Find_Path(LIBVORBIS_INCLUDE_DIR
	vorbis/codec.h
	/usr/include/ /usr/local/include/
	${LIBVORBIS_EXTRALIB_INCLUDE_PATHS}
)

Find_Library(LIBVORBIS_LIBRARY
	vorbis
	/usr/lib /usr/local/lib
	${LIBVORBIS_EXTRALIB_LIBRARY_PATHS}
)

IF(LIBVORBIS_INCLUDE_DIR AND LIBVORBIS_LIBRARY)
	SET(LIBVORBIS_FOUND TRUE)
	MESSAGE(STATUS "Found vorbis library")
ENDIF(LIBVORBIS_INCLUDE_DIR AND LIBVORBIS_LIBRARY)

Find_Path(LIBVORBISFILE_INCLUDE_DIR
	vorbis/vorbisfile.h
	/usr/include/ /usr/local/include/
	${LIBVORBIS_INCLUDE_DIR}
	${LIBVORBIS_EXTRALIB_INCLUDE_PATHS}
)

Find_Library(LIBVORBISFILE_LIBRARY
	vorbisfile
	/usr/lib /usr/local/lib
	${LIBVORBIS_EXTRALIB_LIBRARY_PATHS}
)

IF(LIBVORBISFILE_INCLUDE_DIR AND LIBVORBISFILE_LIBRARY)
	SET(LIBVORBISFILE_FOUND TRUE)
	MESSAGE(STATUS "Found vorbisfile library")
ENDIF(LIBVORBISFILE_INCLUDE_DIR AND LIBVORBISFILE_LIBRARY)
