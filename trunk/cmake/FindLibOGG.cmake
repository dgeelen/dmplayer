SET(LIBOGG_FOUND FALSE)

Find_Path(LIBOGG_INCLUDE_DIR
  ogg/ogg.h
  /usr/include /usr/local/include
  )

Find_Library(LIBOGG_LIBRARY
  ogg
  /usr/lib /usr/local/lib
  )

IF(LIBOGG_INCLUDE_DIR AND LIBOGG_LIBRARY)
  SET(LIBOGG_FOUND TRUE)
  MESSAGE("-- Found libogg")
ENDIF(LIBOGG_INCLUDE_DIR AND LIBOGG_LIBRARY)
