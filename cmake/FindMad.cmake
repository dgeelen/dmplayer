Find_Path(MAD_INCLUDE_DIR
  mad.h
  usr/include usr/local/include
  "C:/Program Files/libmad-0.15.1b/"
  "D:/libmad-0.15.1b/"
  )
  
Find_Library(MAD_LIBRARY
  mad
  usr/lib usr/local/lib
  "C:/Program Files/libmad-0.15.1b/.libs"
  "D:/libmad-0.15.1b/.libs"
  )# Do something about the version?

IF (MAD_INCLUDE_DIR AND MAD_LIBRARY)
  SET(MAD_FOUND TRUE)
  MESSAGE(STATUS "Found Mad library")
ENDIF (MAD_INCLUDE_DIR AND MAD_LIBRARY)
