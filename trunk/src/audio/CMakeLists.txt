# Try to find all packages that we need/support
find_package(PortAudio)
find_package(Mad)
find_package(LibOGG)
find_package(LibVorbis)
find_package(LibFaad2)
find_package(LibFlac)
find_package(LibSamplerate)
find_package(LibDUMB)
find_package(LibSidplay)

ADD_SUBDIRECTORY(zlib)

SET(BACKEND_SOURCE_FILES "")
SET(DECODER_SOURCE_FILES "")
SET(FILTER_SOURCE_FILES "")
SET(BACKEND_LIBRARIES "")
SET(DECODER_LIBRARIES "")
SET(FILTER_LIBRARIES "")
SET(DECODER_DEFINES "")

SET(DECODER_NAMES "")
SET(DECODER_NAMES_STATIC "")
SET(DECODER_NAMES_DYNAMIC "")

SET(MSVC_DEBUG_TAG)
SET(MSVC_OPTIMIZED_TAG)
IF(MSVC)
	SET(MSVC_DEBUG_TAG debug)
	SET(MSVC_OPTIMIZED_TAG optimized)
ENDIF(MSVC)

#First set up everything for all BackEnds
IF(PORTAUDIO_FOUND AND PORTAUDIO_DEBUG_FOUND) #PortAudio backend
	SET(BACKEND_SOURCE_FILES ${BACKEND_SOURCE_FILES}
		backend_portaudio.h
		backend_portaudio.cpp
	)
	INCLUDE_DIRECTORIES(${PORTAUDIO_INCLUDE_DIR})
	ADD_DEFINITIONS(-DPORTAUDIO_BACKEND)
	SET(BACKEND_LIBRARIES ${BACKEND_LIBRARIES} ${MSVC_DEBUG_TAG} ${PORTAUDIO_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${PORTAUDIO_LIBRARY})
ENDIF(PORTAUDIO_FOUND AND PORTAUDIO_DEBUG_FOUND)

SET(ENABLE_MENCODER_DECODER OFF CACHE BOOL "Enable building of the mencoder decoder")

#
# For each decoder it's name should be added to DECODER_NAMES and the following variables can be set:
#   DECODER_${NAME}_SOURCES  : additional sources beyond the default decoder_${lname}.(cpp|h)
#   DECODER_${NAME}_INCLUDES : include directories needed for this decoder
#   DECODER_${NAME}_LIBRARIES: libraries needed for this decoder
#   DECODER_${NAME}_DEFINES  : definitions needed for this decoder
# (note the ${NAME} should be the uppercase version of the decoder name
#

IF(ENABLE_MENCODER_DECODER)
	SET(DECODER_NAMES ${DECODER_NAMES} Mencoder)
	SET(DECODER_MENCODER_SOURCES
		../libexecstream/exec-stream.cpp ../libexecstream/exec-stream.h
	)
ENDIF(ENABLE_MENCODER_DECODER)

# Set up all demuxer before the decoders to prevent the
# decoders from detecting encapsulated data
IF(LIBOGG_FOUND AND LIBOGG_DEBUG_FOUND) # Xiph.org OGG decoder (demuxer really)
	SET(DECODER_NAMES ${DECODER_NAMES} OGG)
	SET(DECODER_OGG_INCLUDES ${LIBOGG_INCLUDE_DIR})
	SET(DECODER_OGG_LIBRARIES ${MSVC_DEBUG_TAG} ${LIBOGG_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBOGG_LIBRARY})
	# The OGG decoder has a bundled datasource, since it
	# is not really a decoder but a demuxer
	SET(DECODER_OGG_SOURCES
		datasource_oggstream.cpp datasource_oggstream.h
	)
ENDIF(LIBOGG_FOUND AND LIBOGG_DEBUG_FOUND)

# Now set up all decoders
IF(LIBFAAD2_FOUND AND LIBFAAD2_DEBUG_FOUND) # http://www.audiocoding.com/faad2.html
	SET(DECODER_NAMES ${DECODER_NAMES} AAC)
	SET(DECODER_AAC_INCLUDES ${LIBFAAD2_INCLUDE_DIR})
	SET(DECODER_AAC_LIBRARIES ${MSVC_DEBUG_TAG} ${LIBFAAD2_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBFAAD2_LIBRARY})
ENDIF(LIBFAAD2_FOUND AND LIBFAAD2_DEBUG_FOUND)

IF(LIBDUMB_FOUND AND LIBDUMB_DEBUG_FOUND) # http://dumb.sourceforge.net/
	SET(DECODER_NAMES ${DECODER_NAMES} DUMB)
	SET(DECODER_DUMB_INCLUDES ${LIBDUMB_INCLUDE_DIR})
	SET(DECODER_DUMB_LIBRARIES ${MSVC_DEBUG_TAG} ${LIBDUMB_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBDUMB_LIBRARY})
ENDIF(LIBDUMB_FOUND AND LIBDUMB_DEBUG_FOUND)

IF(LIBMAD_FOUND AND LIBMAD_DEBUG_FOUND) # MAD MP3 decoder
	SET(DECODER_NAMES ${DECODER_NAMES} Mad)
	SET(DECODER_MAD_INCLUDES ${LIBMAD_INCLUDE_DIR})
	SET(DECODER_MAD_LIBRARIES ${MSVC_DEBUG_TAG} ${LIBMAD_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBMAD_LIBRARY})
ENDIF(LIBMAD_FOUND AND LIBMAD_DEBUG_FOUND)

IF(LIBVORBIS_FOUND AND LIBVORBIS_DEBUG_FOUND) # Xiph.org Vorbis decoder
	SET(DECODER_NAMES ${DECODER_NAMES} Vorbis)
	SET(DECODER_VORBIS_INCLUDES ${LIBVORBIS_INCLUDE_DIR})
	SET(DECODER_VORBIS_LIBRARIES ${MSVC_DEBUG_TAG} ${LIBVORBIS_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBVORBIS_LIBRARY})
ENDIF(LIBVORBIS_FOUND AND LIBVORBIS_DEBUG_FOUND)

IF(LIBVORBISFILE_FOUND AND LIBOGG_FOUND AND LIBOGG_DEBUG_FOUND AND LIBVORBIS_FOUND AND LIBVORBIS_DEBUG_FOUND)
	SET(DECODER_NAMES ${DECODER_NAMES} OGGVorbisFile)
	SET(DECODER_OGGVORBISFILE_INCLUDES
		${LIBOGG_INCLUDE_DIR}
		${LIBVORBIS_INCLUDE_DIR}
		${LIBVORBISFILE_INCLUDE_DIR}
	)
	SET(DECODER_OGGVORBISFILE_LIBRARIES
		${MSVC_DEBUG_TAG}     ${LIBOGG_DEBUG_LIBRARY} ${MSVC_DEBUG_TAG}     ${LIBVORBIS_DEBUG_LIBRARY} ${MSVC_DEBUG_TAG} ${LIBVORBISFILE_DEBUG_LIBRARY}
		${MSVC_OPTIMIZED_TAG} ${LIBOGG_LIBRARY}       ${MSVC_OPTIMIZED_TAG} ${LIBVORBIS_LIBRARY}       ${MSVC_OPTIMIZED_TAG} ${LIBVORBISFILE_LIBRARY}
	)
ENDIF(LIBVORBISFILE_FOUND AND LIBOGG_FOUND AND LIBOGG_DEBUG_FOUND AND LIBVORBIS_FOUND AND LIBVORBIS_DEBUG_FOUND)

IF(LIBFLAC_FOUND AND LIBFLACPP_FOUND AND LIBFLAC_DEBUG_FOUND AND LIBFLACPP_DEBUG_FOUND)
	SET(DECODER_NAMES ${DECODER_NAMES} Flac)
	SET(DECODER_FLAC_INCLUDES ${LIBFLAC_INCLUDE_DIR} ${LIBFLACPP_INCLUDE_DIR})
	SET(DECODER_FLAC_LIBRARIES 
		${MSVC_DEBUG_TAG}     ${LIBFLAC_DEBUG_LIBRARY} ${MSVC_DEBUG_TAG}     ${LIBFLACPP_DEBUG_LIBRARY}
		${MSVC_OPTIMIZED_TAG} ${LIBFLAC_LIBRARY}       ${MSVC_OPTIMIZED_TAG} ${LIBFLACPP_LIBRARY}
	)
	# link statically by default
	SET(DECODER_FLAC_DEFINES FLAC__NO_DLL)
ENDIF(LIBFLAC_FOUND AND LIBFLACPP_FOUND AND LIBFLAC_DEBUG_FOUND AND LIBFLACPP_DEBUG_FOUND)

IF(LIBSIDPLAY_FOUND AND LIBSIDPLAY_DEBUG_FOUND AND LIBSIDPLAY_RESID_BUILDER_FOUND AND LIBSIDPLAY_RESID_BUILDER_DEBUG_FOUND)
	SET(DECODER_NAMES ${DECODER_NAMES} Sidplay)
	SET(DECODER_SIDPLAY_INCLUDES ${LIBSIDPLAY_INCLUDE_DIR} ${LIBSIDPLAY_RESID_BUILDER_INCLUDE_DIR})
	SET(DECODER_SIDPLAY_LIBRARIES
		${MSVC_DEBUG_TAG} ${LIBSIDPLAY_DEBUG_LIBRARY}               ${MSVC_OPTIMIZED_TAG} ${LIBSIDPLAY_LIBRARY}
		${MSVC_DEBUG_TAG} ${LIBSIDPLAY_RESID_BUILDER_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBSIDPLAY_RESID_BUILDER_LIBRARY}
	)
ENDIF(LIBSIDPLAY_FOUND AND LIBSIDPLAY_DEBUG_FOUND AND LIBSIDPLAY_RESID_BUILDER_FOUND AND LIBSIDPLAY_RESID_BUILDER_DEBUG_FOUND)

# we can always compile the wave decoder, no libs needed for that
SET(DECODER_NAMES ${DECODER_NAMES} Wave)

# we can always compile the zlib decoder, we compile our own lib for that
SET(DECODER_NAMES ${DECODER_NAMES} ZLib)
SET(DECODER_ZLIB_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
SET(DECODER_ZLIB_LIBRARIES our_zlib)

# we can always compile the Playlist decoder, no libs needed for that
SET(DECODER_NAMES ${DECODER_NAMES} Playlist)

# optional filters
IF(LIBSAMPLERATE_FOUND AND LIBSAMPLERATE_DEBUG_FOUND)
	SET(FILTER_SOURCE_FILES ${FILTER_SOURCE_FILES}
		filter_libsamplerate.h
		filter_libsamplerate.cpp
	)	
	INCLUDE_DIRECTORIES(${LIBSAMPLERATE_INCLUDE_DIR})
	ADD_DEFINITIONS(-DLIBSAMPLERATE_FILTER)
	SET(FILTER_LIBRARIES ${FILTER_LIBRARIES} ${MSVC_DEBUG_TAG} ${LIBSAMPLERATE_DEBUG_LIBRARY} ${MSVC_OPTIMIZED_TAG} ${LIBSAMPLERATE_LIBRARY})
ENDIF(LIBSAMPLERATE_FOUND AND LIBSAMPLERATE_DEBUG_FOUND)

SET(BUILD_DYNAMIC_DECODERS "" CACHE STRING "List(;) of decoders to build dynamically, 'ALL' for all decoders, prefix with ! to not build statically")

IF("${BUILD_DYNAMIC_DECODERS}" STREQUAL "ALL")
	SET(DECODER_NAMES_DYNAMIC ${DECODER_NAMES})
ELSEIF("${BUILD_DYNAMIC_DECODERS}" STREQUAL "!ALL")
	SET(DECODER_NAMES_DYNAMIC ${DECODER_NAMES})
	SET(DECODER_NAMES "")
ELSE()
	FOREACH(name ${BUILD_DYNAMIC_DECODERS})
		IF("${name}" MATCHES "(!)([A-Za-z0-9]*)")
			LIST(APPEND DECODER_NAMES_DYNAMIC ${CMAKE_MATCH_2})
			LIST(REMOVE_ITEM DECODER_NAMES ${CMAKE_MATCH_2})
		ELSE()
			LIST(APPEND DECODER_NAMES_DYNAMIC ${name})
		ENDIF()
	ENDFOREACH()
ENDIF()

#LIST(APPEND DECODER_NAMES_STATIC )
SET(DECODER_NAMES_STATIC ${DECODER_NAMES} ${DECODER_NAMES_STATIC})
MESSAGE(STATUS "Static Decoders: ${DECODER_NAMES_STATIC}")
MESSAGE(STATUS "Dynamic Decoders: ${DECODER_NAMES_DYNAMIC}")

# configure static decoders
SET(FName ${CMAKE_CURRENT_BINARY_DIR}/decoder_linker.inc)
FILE(WRITE "${FName}" "\n")
#MESSAGE("${FName}")
FOREACH(name ${DECODER_NAMES_STATIC})
	STRING(TOLOWER ${name} lname)
	STRING(TOUPPER ${name} uname)

	# write linking registration
	FILE(APPEND "${FName}" "#include <decoder_${lname}.h>\n")
	FILE(APPEND "${FName}" "REGISTER_DECODER_CLASS(${name}Decoder);\n")

	# add decoder sources
	SET(DECODER_SOURCE_FILES ${DECODER_SOURCE_FILES}
		${DECODER_${uname}_SOURCES}
		decoder_${lname}.h
		decoder_${lname}.cpp
	)

	# add include directories
	INCLUDE_DIRECTORIES(${DECODER_${uname}_INCLUDES})

	# add link libraries
	SET(DECODER_LIBRARIES ${DECODER_LIBRARIES} ${DECODER_${uname}_LIBRARIES})

	# add definitions
	LIST(APPEND DECODER_DEFINES ${DECODER_${uname}_DEFINES})
ENDFOREACH(name)

# configure dynamic decoders
FOREACH(name ${DECODER_NAMES_DYNAMIC})
	STRING(TOLOWER ${name} lname)
	STRING(TOUPPER ${name} uname)

	# add decoder sources
	ADD_LIBRARY(pluginDecoder${name} SHARED
		${DECODER_${uname}_SOURCES}
		decoder_${lname}.h
		decoder_${lname}.cpp
		plugin_helper.h
		plugin_helper.cpp
	)

	# add include directories
	INCLUDE_DIRECTORIES(${DECODER_${uname}_INCLUDES})

	# add link libraries
	TARGET_LINK_LIBRARIES(pluginDecoder${name} ${DECODER_${uname}_LIBRARIES})

	# add definitions
	SET_TARGET_PROPERTIES(pluginDecoder${name} PROPERTIES COMPILE_DEFINITIONS
		"${DECODER_${uname}_DEFINES};DECODER_HEADER=\"decoder_${lname}.h\";DECODER_CLASS=${name}Decoder"
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

	backend_wavwriter.h           backend_wavwriter.cpp

	datasource_filereader.h       datasource_filereader.cpp
	datasource_httpstream.h       datasource_httpstream.cpp
	datasource_resetbuffer.h      datasource_resetbuffer.cpp

	filter_sampledoubler.h        filter_sampledoubler.cpp
	filter_sampleconverter.h      filter_sampleconverter.cpp
	filter_reformat.h             filter_reformat.cpp
	filter_monotostereo.h         filter_monotostereo.cpp
	filter_iir.h                  filter_iir.cpp
	filter_split.h                filter_split.cpp
	filter_normalize.h            filter_normalize.cpp
	filter_downmix.h              filter_downmix.cpp
	filter_upmix.h                filter_upmix.cpp

	${BACKEND_SOURCE_FILES}
	${DECODER_SOURCE_FILES}
	${FILTER_SOURCE_FILES}
)

# Set link properties of mpmp_audio (only needed when ADD_LIBRARY(mpmp_audio) above
# does not contain any source files, but only headers
SET_TARGET_PROPERTIES(mpmp_audio PROPERTIES LINKER_LANGUAGE CXX)
SET_TARGET_PROPERTIES(mpmp_audio PROPERTIES COMPILE_DEFINITIONS
	"${DECODER_DEFINES}"
)

# Define libraries that we link against
TARGET_LINK_LIBRARIES(mpmp_audio
	${BACKEND_LIBRARIES}
	${DECODER_LIBRARIES}
	${FILTER_LIBRARIES}
)

IF(WIN32)
	TARGET_LINK_LIBRARIES(mpmp_audio
		ws2_32
	)
ENDIF(WIN32)
