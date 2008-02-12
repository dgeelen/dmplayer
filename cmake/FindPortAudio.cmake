find_package(ExtraLibDir)

FIND_LIB_IN_EXTRALIBS(LIBPORTAUDIO *portaudio* include lib)

Find_Path(PORTAUDIO_INCLUDE_DIR
	portaudio.h
	usr/include usr/local/include
	"C:/Program Files/portaudio_v18_1/pa_common"
	"D:/portaudio_v18_1/pa_common"
	${LIBPORTAUDIO_EXTRALIB_INCLUDE_PATHS}
)

Find_Library(PORTAUDIO_LIBRARY
	portaudio
	usr/lib usr/local/lib
	"C:/Program Files/portaudio_v18_1/lib"
	"D:/portaudio_v18_1/lib"
	${LIBPORTAUDIO_EXTRALIB_LIBRARY_PATHS}
)

SET(PORTAUDIO_FOUND FALSE)
IF(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY )
	SET(PORTAUDIO_FOUND TRUE)
	IF(WIN32)
		SET(PORTAUDIO_LIBRARY ${PORTAUDIO_LIBRARY} winmm)
	ENDIF(WIN32)
	MESSAGE(STATUS "Found PortAudio Library")
ENDIF(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY )
