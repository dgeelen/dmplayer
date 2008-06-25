STRING(COMPARE EQUAL "${ENABLE_GTK_GUI}" "" AUTODETECT_GTK_GUI)
IF(AUTODETECT_GTK_GUI)
	find_package(PkgConfig)
	pkg_check_modules(GTKMM24 gtkmm-2.4)

	IF(PKG_CONFIG_FOUND AND GTKMM24_FOUND)
		SET(ENABLE_GTK_GUI ON CACHE BOOL "Enable building of the gtk gui")
	ELSE(PKG_CONFIG_FOUND AND GTKMM24_FOUND)
		SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
	ENDIF(PKG_CONFIG_FOUND AND GTKMM24_FOUND)
ENDIF(AUTODETECT_GTK_GUI)

IF(ENABLE_GTK_GUI)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTKMM24 gtkmm-2.4)

	INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR}
	                    ${GTKMM24_INCLUDE_DIRS}
	                   )
	LINK_DIRECTORIES   (${BOOST_LIBRARY_DIR}
	                    ${GTKMM24_LIBRARY_DIRS}
	                   )
	LINK_LIBRARIES     (${BOOST_LIBRARIES}
	                    ${GTKMM24_LIBRARIES}
	                   )

	ADD_EXECUTABLE(gmpmpc
	               gmpmpc.cpp                     gmpmpc.h
	               gmpmpc_trackdb.cpp             gmpmpc_trackdb.h
	               gmpmpc_select_server.cpp       gmpmpc_select_server.h
	               gmpmpc_connection_handling.cpp gmpmpc_connection_handling.h
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
	                      mpmp_audio
	                      mpmp_util
	)

ENDIF(ENABLE_GTK_GUI)
