FIND_PACKAGE(MyBoost)

INCLUDE_DIRECTORIES("${BOOST_INCLUDE_DIR}" )
LINK_DIRECTORIES("${BOOST_LIBRARY_DIR}")


SUBDIRS(gtk-client)
SUBDIRS(qt-client)
SUBDIRS(audio)

ADD_EXECUTABLE(mpmpd
	mpmpd.h mpmpd.cpp
	network-core.h network-core.cpp
	network-handler.h network-handler.cpp
	cross-platform.h
	types.h
)

TARGET_LINK_LIBRARIES(mpmpd
	${BOOST_THREAD_LIBRARY}
	${BOOST_SIGNALS_LIBRARY}
	${BOOST_PROGRAM_OPTIONS_LIBRARY}
)

INCLUDE_DIRECTORIES("D:/Stuff/Projects/dMPlayer/portaudio_v18_1/pa_common")

TARGET_LINK_LIBRARIES(mpmpd
	"D:/Stuff/Projects/dMPlayer/portaudio_v18_1/VC8/lib/PAStaticWMMED.lib"
	winmm
)

IF(WIN32)
	TARGET_LINK_LIBRARIES(mpmpd
		ws2_32
	)
ENDIF(WIN32)
