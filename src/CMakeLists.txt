FIND_PACKAGE(MyBoost)
FIND_PACKAGE(FMod)

INCLUDE_DIRECTORIES("${BOOST_INCLUDE_DIR}")
LINK_DIRECTORIES("${BOOST_LIBRARY_DIR}")

SUBDIRS(gtk-client)
SUBDIRS(qt-client)

ADD_EXECUTABLE(mpmpd
	mpmpd.h mpmpd.cpp
	network-core.h network-core.cpp
	network-handler.h network-handler.cpp
	cross-platform.h
	types.h
	audio/mp3/mp3_interface.h
	audio/mp3/mp3_interface.cpp
	audio/gplayback_interface.h
	audio/gplayback_interface.cpp
)

TARGET_LINK_LIBRARIES(mpmpd
	${BOOST_THREAD_LIBRARY}
	${BOOST_SIGNALS_LIBRARY}
	${BOOST_PROGRAM_OPTIONS_LIBRARY}
	${FMOD_LIBRARY}
)

IF(WIN32)
	GET_TARGET_PROPERTY(EXEPATH mpmpd LOCATION)
	FILE(TO_NATIVE_PATH "${EXEPATH}" EXEPATH)
	STRING(REPLACE mpmpd.exe fmod.dll FMODDLLPATH "${EXEPATH}")
	ADD_CUSTOM_COMMAND(TARGET mpmpd
		POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy ${CMAKE_CURRENT_SOURCE_DIR}/audio/mp3/fmod.dll "${FMODDLLPATH}"
	)
	#CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/audio/mp3/fmod.dll ${FMODDLLPATH}/fmod.dll COPYONLY)
	#MESSAGE(${EXEPATH}/fmod.dll)
ENDIF(WIN32)

IF(WIN32)
	TARGET_LINK_LIBRARIES(mpmpd
		ws2_32
	)
ENDIF(WIN32)
