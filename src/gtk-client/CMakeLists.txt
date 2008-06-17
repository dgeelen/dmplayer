STRING(COMPARE EQUAL "${ENABLE_GTK_GUI}" "" AUTODETECT_GTK_GUI)
IF(AUTODETECT_GTK_GUI)
	find_package(PkgConfig)

# disabling gtkmm as it is only usefull in combination with libglademm, which
# not a single package on my system uses (and so neither shall gmpmpc!). We
# use gtk+-2.0 instead :)
# 	pkg_check_modules(GTKMM gtkmm-2.4)
	pkg_check_modules(GTK+20 gtk+-2.0)
	pkg_check_modules(LIBGLADE libglade-2.0)

	IF(PKG_CONFIG_FOUND AND GTK+20_FOUND AND LIBGLADE_FOUND)
		SET(ENABLE_GTK_GUI ON CACHE BOOL "Enable building of the gtk gui")
	ELSE(PKG_CONFIG_FOUND AND GTK+20_FOUND AND LIBGLADE_FOUND)
		SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
	ENDIF(PKG_CONFIG_FOUND AND GTK+20_FOUND AND LIBGLADE_FOUND)
ENDIF(AUTODETECT_GTK_GUI)

IF(ENABLE_GTK_GUI)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK+20 REQUIRED gtk+-2.0)
	pkg_check_modules(LIBGLADE REQUIRED libglade-2.0)

	# Now the variables GTKMM_INCLUDE_DIRS, GTKMM_LIBRARY_DIRS and GTKMM_LIBRARIES
	# contain what you expect

	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR} ${GTK+20_INCLUDE_DIRS} ${LIBGLADE_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
	LINK_DIRECTORIES   (${BOOST_LIBRARY_DIR} ${GTK+20_LIBRARY_DIRS} ${LIBGLADE_LIBRARY_DIRS}                            )
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
		gmpmpc.cpp gmpmpc.h
		gmpmpc_select_server.h       gmpmpc_select_server.cpp
		gmpmpc_connection_handling.h gmpmpc_connection_handling.cpp
		gmpmpc_playlist.h            gmpmpc_playlist.cpp
		../network-handler.cpp       ../network-handler.h
		../error-handling.cpp        ../error-handling.h
		../packet.cpp                ../packet.h
		../playlist_management.cpp   ../playlist_management.h
		"${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_main_window.glade.h"
		"${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_select_server_window.glade.h"
	)

	TARGET_LINK_LIBRARIES(gmpmpc
		${BOOST_THREAD_LIBRARY}
		${BOOST_SIGNALS_LIBRARY}
		${BOOST_PROGRAM_OPTIONS_LIBRARY}
		${BOOST_SERIALIZATION_LIBRARY}
		${BOOST_FILESYSTEM_LIBRARY}
		${GTKMM_LIBRARIES}
		lib_network
		mpmp_audio
		mpmp_util
	)

ENDIF(ENABLE_GTK_GUI)
