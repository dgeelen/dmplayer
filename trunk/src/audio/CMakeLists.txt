# Try to find all packages that we need/support
find_package(SDL)
find_package(SDL_mixer)
find_package(PortAudio)
find_package(Mad)
find_package(LibAO)
find_package(LibOGG)

SET(BACKEND_SOURCE_FILES "")
SET(DECODER_SOURCE_FILES "")
SET(BACKEND_LIBRARIES "")
SET(DECODER_LIBRARIES "")

#First set up everything for all BackEnds
IF(PORTAUDIO_FOUND) #PortAudio backend
	SET(BACKEND_SOURCE_FILES ${BACKEND_SOURCE_FILES}
		backend_portaudio.h
		backend_portaudio.cpp
	)
	SET(BACKEND_LIBRARIES ${BACKEND_LIBRARIES} ${PORTAUDIO_LIBRARY})
	INCLUDE_DIRECTORIES(${PORTAUDIO_INCLUDE_DIR})
	ADD_DEFINITIONS(-DPORTAUDIO_BACKEND)
ENDIF(PORTAUDIO_FOUND)

IF(SDLMIXER_FOUND) #SDL backend
	SET(BACKEND_SOURCE_FILES ${BACKEND_SOURCE_FILES}
		backend_sdlmixer.h
		backend_sdlmixer.cpp
	)
	SET(BACKEND_LIBRARIES ${BACKEND_LIBRARIES} ${SDLMIXER_LIBRARY})
	INCLUDE_DIRECTORIES(
		${SDL_INCLUDE_DIR}
		${SDLMIXER_INCLUDE_DIR}
	)
	ADD_DEFINITIONS(-DSDL_MIXER_BACKEND)
ENDIF(SDLMIXER_FOUND)

IF(LIBAO_FOUND) # Xiph.org libao backend
	SET(BACKEND_SOURCE_FILES ${BACKEND_SOURCE_FILES}
		backend_libao.h
		backend_libao.cpp
	)
	SET(BACKEND_LIBRARIES ${BACKEND_LIBRARIES} ${LIBAO_LIBRARY})
	INCLUDE_DIRECTORIES(
		${LIBAO_INCLUDE_DIR}
	)
	ADD_DEFINITIONS(-DLIBAO_BACKEND)
ENDIF(LIBAO_FOUND)

# Now set up all decoders
IF(MAD_FOUND) # MAD MP3 decoder
	SET(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES}
		decoder_mad.h
		decoder_mad.cpp
	)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${MAD_LIBRARY})
	INCLUDE_DIRECTORIES(${MAD_INCLUDE_DIR})
	ADD_DEFINITIONS(-DMAD_DECODER)
ENDIF(MAD_FOUND)

IF(LIBOGG_FOUND) # Xiph.org OGG decoder (demuxer really)
	SET(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES}
		decoder_libogg.h
		decoder_libogg.cpp
	)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${LIBOGG_LIBRARY})
	INCLUDE_DIRECTORIES(${LIBOGG_INCLUDE_DIR})
	ADD_DEFINITIONS(-DLIBOGG_DECODER)
ENDIF(LIBOGG_FOUND)

# we can always compile the raw decoder, no libs needed for that
SET(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES}
	decoder_raw.h
	decoder_raw.cpp
)
#ADD_DEFINITIONS(-DRAW_DECODER) // we should not need definitions for these

# Finally declare all files to CMake
ADD_LIBRARY(mpmp_audio
	backend_interface.h
	datasource_interface.h
	decoder_interface.h
	audio_controller.h
	audio_controller.cpp
	datasource_filereader.h
	datasource_filereader.cpp

	${BACKEND_SOURCE_FILES}
	${DECODER_SOURCE_FILES}
)

# Set link properties of mpmp_audio (only needed when ADD_LIBRARY(mpmp_audio) above
# does not contain any source files, but only headers
SET_TARGET_PROPERTIES(mpmp_audio PROPERTIES LINKER_LANGUAGE CXX)

# Define libraries that we link against
TARGET_LINK_LIBRARIES(mpmp_audio
	${BACKEND_LIBRARIES}
	${BACKEND_LIBRARIES}
)
