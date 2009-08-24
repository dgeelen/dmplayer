STRING(COMPARE EQUAL "${ENABLE_GTK_GUI}" "" AUTODETECT_GTK_GUI)
IF(AUTODETECT_GTK_GUI)
	find_package(PkgConfig)
	pkg_check_modules(GTKMM24 gtkmm-2.4)
	pkg_check_modules(GTHREAD gthread-2.0)

	IF(PKG_CONFIG_FOUND)
		IF(GTKMM24_FOUND)
			IF(GTHREAD_FOUND)
				MESSAGE(STATUS "Enabling gtk-gui")
				SET(ENABLE_GTK_GUI ON CACHE BOOL "Enable building of the gtk gui")
				IF(WIN32)
					               #1234567890          1234567890          1234567890          1234567890
					               #          1234567890          1234567890          1234567890          1234567890
					MESSAGE(STATUS "NOTE: You will still need to manually set GTKMM24_INCLUDE_DIRS and")
					MESSAGE(STATUS "GTHREAD_INCLUDE_DIRS, since pkg-config is broken on WIN32: it will A) generate")
					MESSAGE(STATUS "CFLAGS only for gnu-tools; and B) can not handle path's with spaces in them")
					MESSAGE(STATUS "(e.g. gtkmm's default installation path ``c:\\progam files\\gtkmm\\'' will cause")
					MESSAGE(STATUS "build failures due to bogus include directives like ``-IC:/Program'').")
					MESSAGE(STATUS "If you are using MSVC there are some Property Sheets in gtkmm's ``MSVC''")
					MESSAGE(STATUS "directory that you may find usefull.")
				ENDIF(WIN32)
			ELSE(GTHREAD_FOUND)
				MESSAGE(STATUS "Missing GThread, disabling gtk-gui")
				SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
			ENDIF(GTHREAD_FOUND)
		ELSE(GTKMM24_FOUND)
			MESSAGE(STATUS "Missing Gtkmm, disabling gtk-gui")
			SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
		ENDIF(GTKMM24_FOUND)
	ELSE(PKG_CONFIG_FOUND)
		MESSAGE(STATUS "Missing PkgConfig, cannot search for gtkmm and gthread, disabling gtk-gui")
		SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
	ENDIF(PKG_CONFIG_FOUND)
ENDIF(AUTODETECT_GTK_GUI)

IF(ENABLE_GTK_GUI)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTKMM24 gtkmm-2.4 REQUIRED)
	pkg_check_modules(GTHREAD gthread-2.0 REQUIRED)

#	MESSAGE(STATUS "GTKMM24_INCLUDE_DIRS: ${GTKMM24_INCLUDE_DIRS}")
#	MESSAGE(STATUS "GTHREAD_INCLUDE_DIRS: ${GTHREAD_INCLUDE_DIRS}")

	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR}
	                    ${GTKMM24_INCLUDE_DIRS}
	                    ${GTHREAD_INCLUDE_DIRS}
	                    ${CMAKE_CURRENT_SOURCE_DIR} # For boost/preprocessor/iteration
	                   )
	LINK_DIRECTORIES   (${BOOST_LIBRARY_DIR}
	                    ${GTKMM24_LIBRARY_DIRS}
	                    ${GTHREAD_LIBRARY_DIRS}
	                   )
	LINK_LIBRARIES     (${BOOST_LIBRARIES}
	                    ${GTKMM24_LIBRARIES}
	                    ${GTHREAD_LIBRARIES}
	                   )

	ADD_EXECUTABLE(gmpmpc
	               gmpmpc.cpp                     gmpmpc.h
	               gmpmpc_trackdb.cpp             gmpmpc_trackdb.h
	               gmpmpc_select_server.cpp       gmpmpc_select_server.h
	               gmpmpc_connection_handling.cpp gmpmpc_connection_handling.h
	               track_treeview.cpp             track_treeview.h
	               gmpmpc_playlist.cpp            gmpmpc_playlist.h
	               gmpmpc_icon.h
	               dispatcher_marshaller.h
	               ../middle_end.cpp              ../middle_end.h
	               ../error-handling.cpp          ../error-handling.h
	               ../network-handler.cpp         ../network-handler.h
	               ../playlist_management.cpp     ../playlist_management.h
	               ../packet.cpp                  ../packet.h
	)

	TARGET_LINK_LIBRARIES(gmpmpc
	                      ${BOOST_THREAD_LIBRARY}
	                      ${BOOST_SIGNALS_LIBRARY}
	                      ${BOOST_PROGRAM_OPTIONS_LIBRARY}
	                      ${BOOST_SERIALIZATION_LIBRARY}
	                      ${BOOST_FILESYSTEM_LIBRARY}
	                      lib_network
#	                      mpmp_audio
	                      mpmp_util
	)

	IF(WIN32)
		TARGET_LINK_LIBRARIES(gmpmpc
			ws2_32
		)
	ENDIF(WIN32)

ENDIF(ENABLE_GTK_GUI)
