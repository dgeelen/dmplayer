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


	IF(MSVC)
		IF(MSVC80)
			find_file(GTKMM_PROPERTY_SHEET       "gtkmm-vc80-2_4.vsprops"   "$ENV{GTKMM_BASEPATH}\\MSVC\\")
			find_file(GTKMM_DEBUG_PROPERTY_SHEET "gtkmm-vc80-d-2_4.vsprops" "$ENV{GTKMM_BASEPATH}\\MSVC\\")
		ELSE(MSVC80)
		IF(MSVC90)
			find_file(GTKMM_PROPERTY_SHEET       "gtkmm-vc90-2_4.vsprops"   "$ENV{GTKMM_BASEPATH}\\MSVC\\")
			find_file(GTKMM_DEBUG_PROPERTY_SHEET "gtkmm-vc90-d-2_4.vsprops" "$ENV{GTKMM_BASEPATH}\\MSVC\\")
		ELSE(MSVC90)
			find_file(GTKMM_PROPERTY_SHEET       "gtkmm-2_4.vsprops"        "$ENV{GTKMM_BASEPATH}\\MSVC\\")
			find_file(GTKMM_DEBUG_PROPERTY_SHEET "gtkmm-d-2_4.vsprops"      "$ENV{GTKMM_BASEPATH}\\MSVC\\")
		ENDIF(MSVC90)
		ENDIF(MSVC80)
		IF(CMAKE_BUILD_TYPE MATCHES Debug)
			SET(_PROPERTY_SHEET ${GTKMM_DEBUG_PROPERTY_SHEET})
		ELSE(CMAKE_BUILD_TYPE MATCHES Debug)
			SET(_PROPERTY_SHEET ${GTKMM_PROPERTY_SHEET})
		ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

		file(STRINGS "${_PROPERTY_SHEET}" GTKMM_ADDITIONAL_INCLUDE_DIRECTORIES REGEX AdditionalIncludeDirectories)
		file(STRINGS "${_PROPERTY_SHEET}" GTKMM_ADDITIONAL_DEPENDENCIES REGEX AdditionalDependencies)
		file(STRINGS "${_PROPERTY_SHEET}" GTKMM_ADDITIONAL_LIBRARY_DIRECTORIES REGEX AdditionalLibraryDirectories)
		STRING(REGEX REPLACE "\\\\" "\\\\\\\\" GTKMM_BASEPATH_REGEX $ENV{GTKMM_BASEPATH})
		FOREACH(S GTKMM_ADDITIONAL_INCLUDE_DIRECTORIES GTKMM_ADDITIONAL_DEPENDENCIES GTKMM_ADDITIONAL_LIBRARY_DIRECTORIES)
			STRING(REGEX REPLACE ^.*AdditionalIncludeDirectories[^=]*=[^\"]*\"   ""                        "${S}" "${${S}}")
			STRING(REGEX REPLACE ^.*AdditionalDependencies[^=]*=[^\"]*\"         ""                        "${S}" "${${S}}")
			STRING(REGEX REPLACE ^.*AdditionalLibraryDirectories[^=]*=[^\"]*\"   ""                        "${S}" "${${S}}")
			STRING(REGEX REPLACE \"[^\"]*$                                       ""                        "${S}" "${${S}}")
			STRING(REGEX REPLACE "\\$\\(GTKMM_BASEPATH\\)"                       ${GTKMM_BASEPATH_REGEX}   "${S}" "${${S}}")
			STRING(REGEX REPLACE "\\\\;"                                         ";"                       "${S}" "${${S}}")
			STRING(REGEX REPLACE "&quot\\;"                                      ""                        "${S}" "${${S}}")
		ENDFOREACH(S)
		SET(GTKMM24_INCLUDE_DIRS "${GTKMM_ADDITIONAL_INCLUDE_DIRECTORIES}")
		SET(GTKMM24_LIBRARIES    "${GTKMM_ADDITIONAL_DEPENDENCIES}")
		SET(GTKMM24_LIBRARY_DIRS "${GTKMM_ADDITIONAL_LIBRARY_DIRECTORIES}")
	ENDIF(MSVC)

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
	
	IF(MSVC)
		file(STRINGS "${_PROPERTY_SHEET}" GTKMM_DISABLE_SPECIFIC_WARNINGS REGEX DisableSpecificWarnings)
		STRING(REGEX REPLACE ^.*DisableSpecificWarnings[^=]*=[^\"]*\" ";"   "GTKMM_DISABLE_SPECIFIC_WARNINGS" "${GTKMM_DISABLE_SPECIFIC_WARNINGS}")
		STRING(REGEX REPLACE \"[^\"]*$                                ""    "GTKMM_DISABLE_SPECIFIC_WARNINGS" "${GTKMM_DISABLE_SPECIFIC_WARNINGS}")
		STRING(REGEX REPLACE "\\\\;"                                  ";"   "GTKMM_DISABLE_SPECIFIC_WARNINGS" "${GTKMM_DISABLE_SPECIFIC_WARNINGS}")
		STRING(REGEX REPLACE "\\;"                                    " /wd" "GTKMM_DISABLE_SPECIFIC_WARNINGS" "${GTKMM_DISABLE_SPECIFIC_WARNINGS}")
		SET_TARGET_PROPERTIES(gmpmpc PROPERTIES COMPILE_FLAGS "${GTKMM_DISABLE_SPECIFIC_WARNINGS}")
	ENDIF(MSVC)

ENDIF(ENABLE_GTK_GUI)
