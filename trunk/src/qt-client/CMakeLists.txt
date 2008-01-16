STRING(COMPARE EQUAL "${ENABLE_QT_GUI}" "" AUTODETECT_QT_GUI)
IF(AUTODETECT_QT_GUI)
	find_package(Qt4)

	IF(QT4_FOUND)
		SET(ENABLE_QT_GUI ON CACHE BOOL "Enable building of the Qt gui")
	ELSE(QT4_FOUND)
		SET(ENABLE_QT_GUI OFF CACHE BOOL "Enable building of the Qt gui")
	ENDIF(QT4_FOUND)
ENDIF(AUTODETECT_QT_GUI)

IF(ENABLE_QT_GUI)
	# CMake >= 2.4.0 is needed for Qt4
	cmake_minimum_required(VERSION 2.4.0) 

	# find and setup Qt4 for this project
	find_package(Qt4 REQUIRED)

	# add definitions needed for Qt4
	add_definitions("${QT_DEFINITIONS} -DQT_NO_KEYWORDS")

	# include Qt4 cmake file
	include(${QT_USE_FILE})

	# add binary dir to include path so we can find the Qt4 generated headers
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	# CMake fails to add all libraries needed for linking with staticially built Qt, so we do it ourselves
	# TODO: maybe this is needed for msvc too
	IF(MINGW AND NOT QT_SHARED) 
		SET(QT_LIBRARIES "${QT_LIBRARIES}" Imm32 Winmm gdi32 comdlg32 oleaut32 winspool Ws2_32 ole32 uuid)
	ENDIF(MINGW AND NOT QT_SHARED)
	
  INCLUDE_DIRECTORIES(${FMOD_INCLUDE_DIR})


	# List .ui files which need processing by Qt4
	SET(UI_FILES
		MainWindow.ui
	)

	# List source files, which may contain .moc includes
	SET(SRC_FILES
		Main.cpp
		MainWindow.cpp
		../audio/mp3/mp3_interface.cpp
		../audio/gplayback_interface.cpp
	)

	# List other files used
	SET(OTHER_FILES
		MainWindow.h
		../audio/gplayback_interface.h
		../audio/mp3/mp3_interface.h
		../network-handler.h
		../network-handler.cpp
		../network-core.cpp
	)

	# Generate headers from .ui files
	QT4_WRAP_UI(UI_H_FILES ${UI_FILES})

	# Generate .moc files as needed
	QT4_AUTOMOC(${SRC_FILES})
	
	QT4_ADD_RESOURCES(ResourceFiles icons.qrc)

	# List sources needed by our executable
	ADD_EXECUTABLE(qt-simple-test
		${SRC_FILES}
		${UI_FILES}
		${OTHER_FILES}
		${ResourceFiles}
		${FMOD_INCLUDE_DIR}
	)

	# Link our executable against needed Qt4 libraries
	TARGET_LINK_LIBRARIES(qt-simple-test
		${QT_LIBRARIES}
		${FMOD_LIBRARY}
		${BOOST_SIGNALS_LIBRARY}
		${BOOST_THREAD_LIBRARY}
	)


ENDIF(ENABLE_QT_GUI)
