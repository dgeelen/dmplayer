find_package(PortAudio)
IF(PORTAUDIO_FOUND)
	SET(BACKEND_FILES_PORTAUDIO
		backend_portaudio.h
		backend_portaudio.cpp
	)
ENDIF(PORTAUDIO_FOUND)

find_package(SDLMixer)
IF(SDLMIXER_FOUND)
	SET(BACKEND_FILES_SDLMIXER
		backend_sdlmixer.h
		backend_sdlmixer.cpp
	)
ENDIF(SDLMIXER_FOUND)

ADD_LIBRARY(mpmp_audio SHARED
	backend_interface.h
	datasource_interface.h
	decoder_interface.h
	${BACKEND_FILES_PORTAUDIO}
	${BACKEND_FILES_SDLMIXER}
)

SET_TARGET_PROPERTIES(mpmp_audio PROPERTIES LINKER_LANGUAGE CXX)
