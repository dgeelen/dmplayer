SET(LIBVORBIS_FOUND FALSE)

Find_Path(LIBVORBIS_INCLUDE_DIR
  vorbis/codec.h
  /usr/include/ /usr/local/include/
  )

Find_Library(LIBVORBIS_LIBRARY
  vorbis
  /usr/lib /usr/local/lib
  )

IF(LIBVORBIS_INCLUDE_DIR AND LIBVORBIS_LIBRARY)
  SET(LIBVORBIS_FOUND TRUE)
  MESSAGE("-- Found libvorbis")
ENDIF(LIBVORBIS_INCLUDE_DIR AND LIBVORBIS_LIBRARY)
