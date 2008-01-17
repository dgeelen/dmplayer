find_package(SDL)
find_package(SDL_mixer)
find_package(PortAudio)


IF(PORTAUDIO_FOUND)
	SET(BACKEND_FILES_PORTAUDIO
		backend_portaudio.h
		backend_portaudio.cpp
	)
	SET(BACKEND_LIBRARIES_PORTAUDIO "")
ENDIF(PORTAUDIO_FOUND)

IF(SDLMIXER_FOUND)
	SET(BACKEND_FILES_SDLMIXER
		backend_sdlmixer.h
		backend_sdlmixer.cpp
	)
	SET(BACKEND_LIBRARIES_SDL ${SDLMIXER_LIBRARY})
ENDIF(SDLMIXER_FOUND)

ADD_LIBRARY(mpmp_audio SHARED
	backend_interface.h
	datasource_interface.h
	decoder_interface.h
	${BACKEND_FILES_PORTAUDIO}
	${BACKEND_FILES_SDLMIXER}
)

SET_TARGET_PROPERTIES(mpmp_audio PROPERTIES LINKER_LANGUAGE CXX)

TARGET_LINK_LIBRARIES(mpmp_audio
	${BACKEND_SDL_LIBRARIES}
	${BACKEND_LIBRARIES_PORTAUDIO}
)
