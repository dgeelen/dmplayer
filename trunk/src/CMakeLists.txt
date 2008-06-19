FIND_PACKAGE(MyBoost)
FIND_PACKAGE(PortAudio)

INCLUDE_DIRECTORIES("${BOOST_INCLUDE_DIR}")
LINK_DIRECTORIES("${BOOST_LIBRARY_DIR}")

SUBDIRS(util)
SUBDIRS(gtk-client)
SUBDIRS(qt-client)
SUBDIRS(audio)
SUBDIRS(network)


ADD_EXECUTABLE(mpmpd
	mpmpd.h mpmpd.cpp
	network-handler.h network-handler.cpp
	synced_playlist.h
	cross-platform.h
	error-handling.h error-handling.cpp
	playlist_management.h playlist_management.cpp
	types.h
	libexecstream/exec-stream.cpp
	packet.h packet.cpp
)

TARGET_LINK_LIBRARIES(mpmpd
	${BOOST_THREAD_LIBRARY}
	${BOOST_SIGNALS_LIBRARY}
	${BOOST_PROGRAM_OPTIONS_LIBRARY}
	${BOOST_FILESYSTEM_LIBRARY}
	${BOOST_SERIALIZATION_LIBRARY}
	mpmp_audio
	mpmp_util
	lib_network
)

INCLUDE_DIRECTORIES("D:/Stuff/Projects/dMPlayer/portaudio_v18_1/pa_common")
INCLUDE_DIRECTORIES("libexecstream")

IF(WIN32)
	TARGET_LINK_LIBRARIES(mpmpd
		ws2_32
	)
ENDIF(WIN32)
