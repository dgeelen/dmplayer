# Try to find all packages that we need/support
find_package(SDL)
find_package(SDL_mixer)
find_package(PortAudio)
find_package(Mad)
find_package(LibAO)
find_package(LibOGG)
find_package(LibVorbis)
find_package(LibFaad2)

SET(BACKEND_SOURCE_FILES "")
SET(DECODER_SOURCE_FILES "")
SET(DECODER_NAMES "")
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

IF(LIBFAAD2_FOUND) # http://www.audiocoding.com/faad2.html
	SET(DECODER_NAMES ${DECODER_NAMES} AAC)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${LIBFAAD2_LIBRARY})
	INCLUDE_DIRECTORIES(${LIBFAAD2_INCLUDE_DIR})
ENDIF(LIBFAAD2_FOUND)

IF(MAD_FOUND) # MAD MP3 decoder
	SET(DECODER_NAMES ${DECODER_NAMES} Mad)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${MAD_LIBRARY})
	INCLUDE_DIRECTORIES(${MAD_INCLUDE_DIR})
ENDIF(MAD_FOUND)

IF(LIBOGG_FOUND) # Xiph.org OGG decoder (demuxer really)
	SET(DECODER_NAMES ${DECODER_NAMES} OGG)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${LIBOGG_LIBRARY})
	INCLUDE_DIRECTORIES(${LIBOGG_INCLUDE_DIR})
	# The OGG decoder has a bundled datasource, since it
	# is not really a decoder but a demuxer
	SET(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES}
		datasource_oggstream.cpp datasource_oggstream.h
	)
ENDIF(LIBOGG_FOUND)

IF(LIBVORBIS_FOUND) # Xiph.org Vorbis decoder
	SET(DECODER_NAMES ${DECODER_NAMES} Vorbis)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${LIBVORBIS_LIBRARY})
	INCLUDE_DIRECTORIES(${LIBVORBIS_INCLUDE_DIR})
ENDIF(LIBVORBIS_FOUND)

IF(LIBVORBISFILE_FOUND AND LIBOGG_FOUND AND LIBVORBIS_FOUND)
	SET(DECODER_NAMES ${DECODER_NAMES} OGGVorbisFile)
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${LIBOGG_LIBRARY} ${LIBVORBIS_LIBRARY} ${LIBVORBISFILE_LIBRARY})
	INCLUDE_DIRECTORIES(${LIBOGG_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${LIBVORBIS_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${LIBVORBISFILE_INCLUDE_DIR})
ENDIF(LIBVORBISFILE_FOUND AND LIBOGG_FOUND AND LIBVORBIS_FOUND)

# we can always compile the wave decoder, no libs needed for that
SET(DECODER_NAMES ${DECODER_NAMES} Wave)

# configure all decoders
SET(FName ${CMAKE_CURRENT_BINARY_DIR}/decoder_linker.inc)
FILE(WRITE "${FName}" "\n")
#MESSAGE("${FName}")
FOREACH(name ${DECODER_NAMES})
	STRING(TOLOWER ${name} lname)
	FILE(APPEND "${FName}" "#include <decoder_${lname}.h>\n")
	FILE(APPEND "${FName}" "REGISTER_DECODER_CLASS(${name}Decoder);\n")
	SET(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES}
		decoder_${lname}.h
		decoder_${lname}.cpp
	)
ENDFOREACH(name)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Finally declare all files to CMake
ADD_LIBRARY(mpmp_audio
	audiosource_interface.h
	backend_interface.h
	datasource_interface.h
	decoder_interface.h           decoder_interface.cpp
	
	audio_controller.h            audio_controller.cpp
	
	datasource_filereader.h       datasource_filereader.cpp
	datasource_httpstream.h       datasource_httpstream.cpp
	
	filter_sampledoubler.h        filter_sampledoubler.cpp
	filter_reformat.h             filter_reformat.cpp
	filter_monotostereo.h         filter_monotostereo.cpp

	${BACKEND_SOURCE_FILES}
	${DECODER_SOURCE_FILES}
)

# Set link properties of mpmp_audio (only needed when ADD_LIBRARY(mpmp_audio) above
# does not contain any source files, but only headers
SET_TARGET_PROPERTIES(mpmp_audio PROPERTIES LINKER_LANGUAGE CXX)

# Define libraries that we link against
TARGET_LINK_LIBRARIES(mpmp_audio
	${BACKEND_LIBRARIES}
	${DECODER_LIBRARIES}
)

IF(WIN32)
	TARGET_LINK_LIBRARIES(mpmp_audio
		ws2_32
	)
ENDIF(WIN32)
