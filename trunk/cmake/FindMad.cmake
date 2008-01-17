Find_Path(MAD_INCLUDE_DIR
  mad.h
  usr/include usr/local/include
  "C:/Program Files/libmad-0.15.1b/"
  "D:/portaudio_v18_1/"
  )
  
Find_Library(MAD_LIBRARY
  mad
  usr/lib usr/local/lib
  "C:/Program Files/libmad-0.15.1b/.libs"
  "D:/portaudio_v18_1/.libs"
  )# Do something about the version?
