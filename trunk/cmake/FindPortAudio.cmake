Find_Path(PORTAUDIO_INCLUDE_DIR
  portaudio.h
  usr/include usr/local/include
  "C:/Program Files/portaudio_v18_1/pa_common"
  "D:/portaudio_v18_1/pa_common"
  )

Find_Library(PORTAUDIO_LIBRARY
  portaudio
  usr/lib usr/local/lib
  "C:/Program Files/portaudio_v18_1/lib"
  "D:/portaudio_v18_1/lib"
  )# Do something about the version of portaudio?

SET(PORTAUDIO_FOUND FALSE)
IF(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY )
  SET(PORTAUDIO_FOUND TRUE)
  IF(WIN32)
    SET(PORTAUDIO_LIBRARY ${PORTAUDIO_LIBRARY} winmm)
  ENDIF(WIN32)
  MESSAGE(STATUS "Found PortAudio Library")
ENDIF(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY )
