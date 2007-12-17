IF(NOT ENABLE_QT_GUI)
	SET(ENABLE_QT_GUI "NO" CACHE String "Enable building of the Qt gui")
ENDIF(NOT ENABLE_QT_GUI)

STRING(COMPARE EQUAL "${ENABLE_QT_GUI}" "YES" INTERNAL_QT_ENABLE)
IF(INTERNAL_QT_ENABLE)

	# CMake >= 2.4.0 is needed for Qt4
	cmake_minimum_required(VERSION 2.4.0) 

	# find and setup Qt4 for this project
	find_package(Qt4 REQUIRED)

	# add definitions needed for Qt4
	add_definitions(${QT_DEFINITIONS})

	# include Qt4 cmake file
	include(${QT_USE_FILE})

	# add binary dir to include path so we can find the Qt4 generated headers
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	# CMake fails to add all libraries needed for linking with staticially built Qt, so we do it ourselves
	# TODO: maybe this is needed for msvc too
	IF(MINGW AND NOT QT_SHARED) 
		SET(QT_LIBRARIES "${QT_LIBRARIES}" Imm32 Winmm gdi32 comdlg32 oleaut32 winspool Ws2_32 ole32 uuid)
	ENDIF(MINGW AND NOT QT_SHARED)


	# List .ui files which need processing by Qt4
	SET(UI_FILES
		MainWindow.ui
	)

	# List source files, which may contain .moc includes
	SET(SRC_FILES
		Main.cpp
		MainWindow.cpp
	)

	# List other files used
	SET(OTHER_FILES
		MainWindow.h
	)

	# Generate headers from .ui files
	QT4_WRAP_UI(UI_H_FILES ${UI_FILES})

	# Generate .moc files as needed
	QT4_AUTOMOC(${SRC_FILES})

	# List sources needed by our executable
	ADD_EXECUTABLE(qt-simple-test
		${SRC_FILES}
		${UI_FILES}
		${OTHER_FILES}
	)

	# Link our executable against needed Qt4 libraries
	TARGET_LINK_LIBRARIES(qt-simple-test
		${QT_LIBRARIES}
	)

ENDIF(INTERNAL_QT_ENABLE)
