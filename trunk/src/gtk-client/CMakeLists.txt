STRING(COMPARE EQUAL "${ENABLE_GTK_GUI}" "" AUTODETECT_GTK_GUI)
IF(AUTODETECT_GTK_GUI)
	find_package(PkgConfig)
	pkg_check_modules(GTKMM gtkmm-2.4)
	pkg_check_modules(LIBGLADE libglade-2.0)

	IF(PKG_CONFIG_FOUND AND GTKMM_FOUND AND LIBGLADE_FOUND)
		SET(ENABLE_GTK_GUI ON CACHE BOOL "Enable building of the gtk gui")
	ELSE(PKG_CONFIG_FOUND AND GTKMM_FOUND AND LIBGLADE_FOUND)
		SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
	ENDIF(PKG_CONFIG_FOUND AND GTKMM_FOUND AND LIBGLADE_FOUND)
ENDIF(AUTODETECT_GTK_GUI)

IF(ENABLE_GTK_GUI)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTKMM REQUIRED gtkmm-2.4)
	pkg_check_modules(LIBGLADE REQUIRED libglade-2.0)

	# Now the variables GTKMM_INCLUDE_DIRS, GTKMM_LIBRARY_DIRS and GTKMM_LIBRARIES
	# contain what you expect

	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR} ${GTKMM_INCLUDE_DIRS} ${LIBGLADE_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
	LINK_DIRECTORIES   (${BOOST_LIBRARY_DIR} ${GTKMM_LIBRARY_DIRS} ${LIBGLADE_LIBRARY_DIRS}                            )
	LINK_LIBRARIES     (                                           ${LIBGLADE_LIBRARIES}                               )

	ADD_CUSTOM_COMMAND(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_main_window.glade.h"
		COMMAND sh ARGS
			"${CMAKE_CURRENT_SOURCE_DIR}/gladegen.sh"
			"${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_main_window.glade"
			"${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_main_window.glade.h"
		MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_main_window.glade"
		COMMENT "Generating: Glade/xml -> C/header"
		VERBATIM
	)

	ADD_CUSTOM_COMMAND(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_select_server_window.glade.h"
		COMMAND sh ARGS
			"${CMAKE_CURRENT_SOURCE_DIR}/gladegen.sh"
			"${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_select_server_window.glade"
			"${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_select_server_window.glade.h"
		MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_select_server_window.glade"
		COMMENT "Generating: Glade/xml -> C/header"
		VERBATIM
	)

	ADD_EXECUTABLE(gmpmpc
		gmpmpc.cpp
		gmpmpc_select_server.cpp
		"${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_main_window.glade.h"
		"${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_select_server_window.glade.h"
	)

	TARGET_LINK_LIBRARIES(gmpmpc
		${BOOST_THREAD_LIBRARY}
		${BOOST_PROGRAM_OPTIONS_LIBRARY}
		${GTKMM_LIBRARIES}
	)

ENDIF(ENABLE_GTK_GUI)
