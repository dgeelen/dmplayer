FIND_PACKAGE(MyBoost)

INCLUDE_DIRECTORIES("${BOOST_INCLUDE_DIR}")
LINK_DIRECTORIES("${BOOST_LIBRARY_DIR}")

#SUBDIRS(gtk-client)

ADD_EXECUTABLE(mpmpd
	mpmpd.h mpmpd.cpp
	network-core.h network-core.cpp
	network-handler.h network-handler.cpp
)

TARGET_LINK_LIBRARIES(mpmpd
	${BOOST_THREAD_LIBRARY}
	${BOOST_PROGRAM_OPTIONS_LIBRARY}
)

IF(WIN32)
	TARGET_LINK_LIBRARIES(mpmpd
		ws2_32
	)
ENDIF(WIN32)
