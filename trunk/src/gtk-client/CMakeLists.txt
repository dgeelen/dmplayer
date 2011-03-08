STRING(COMPARE EQUAL "${ENABLE_GTK_GUI}" "" AUTODETECT_GTK_GUI)
IF(AUTODETECT_GTK_GUI)
	find_package(PkgConfig)
	pkg_check_modules(GTKMM24 gtkmm-2.4)
	pkg_check_modules(GTHREAD gthread-2.0)
	pkg_check_modules(PIXBUF gdk-pixbuf-2.0)

	IF(PKG_CONFIG_FOUND)
		IF(GTKMM24_FOUND)
			IF(GTHREAD_FOUND)
				IF(PIXBUF_FOUND)
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
				ELSE(PIXBUF_FOUND)
					MESSAGE(STATUS "Missing gdk-pixbuf, disabling gtk-gui")
					SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
				ENDIF(PIXBUF_FOUND)
			ELSE(GTHREAD_FOUND)
				MESSAGE(STATUS "Missing GThread, disabling gtk-gui")
				SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
			ENDIF(GTHREAD_FOUND)
		ELSE(GTKMM24_FOUND)
			MESSAGE(STATUS "Missing Gtkmm, disabling gtk-gui")
			SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
		ENDIF(GTKMM24_FOUND)
	ELSE(PKG_CONFIG_FOUND)
		MESSAGE(STATUS "Missing PkgConfig, cannot search for gtkmm, gthread and gdk-pixbuf, disabling gtk-gui")
		SET(ENABLE_GTK_GUI OFF CACHE BOOL "Enable building of the gtk gui")
	ENDIF(PKG_CONFIG_FOUND)
ENDIF(AUTODETECT_GTK_GUI)

SET(GENERATE_GMPMPC_ICON_FROM_SOURCE_PNG_INSTEAD_OF_SVG_BECAUSE_OF_B0RKEN_SVG_SUPPORT_IN_IMAGEMAGICK True CACHE BOOL "Work around bugs in ImageMagick's SVG module")

IF(ENABLE_GTK_GUI)
	find_package(PkgConfig REQUIRED)
	find_package(ImageMagick COMPONENTS convert)
	find_package(SearchUtils REQUIRED)
	GLOB_PATHS(GLOB_PATHS NOBASERESULT GLOBS "*" BASES "${CMAKE_EXTRA_LIBS_PATH}")
	FOREACH(GLOB_PATH ${GLOB_PATHS})
		IF(NOT PNG2ICO_EXECUTABLE)
			find_program(PNG2ICO_EXECUTABLE NAMES png2ico png2ico.exe PATHS "${GLOB_PATH}")
		ENDIF(NOT PNG2ICO_EXECUTABLE)
	ENDFOREACH(GLOB_PATH)
	pkg_check_modules(GTKMM24 gtkmm-2.4      REQUIRED)
	pkg_check_modules(GTHREAD gthread-2.0    REQUIRED)
	pkg_check_modules(PIXBUF  gdk-pixbuf-2.0 REQUIRED)

	MACRO(GENERATE_ICON_IMAGE SIZE BPP COLOR_COUNT)
		IF(GENERATE_GMPMPC_ICON_FROM_SOURCE_PNG_INSTEAD_OF_SVG_BECAUSE_OF_B0RKEN_SVG_SUPPORT_IN_IMAGEMAGICK)
			SET(_GMPMPC_ICON_SOURCE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_icon.png")
		ELSE(GENERATE_GMPMPC_ICON_FROM_SOURCE_PNG_INSTEAD_OF_SVG_BECAUSE_OF_B0RKEN_SVG_SUPPORT_IN_IMAGEMAGICK)
			SET(_GMPMPC_ICON_SOURCE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_icon.svg")
		ENDIF(GENERATE_GMPMPC_ICON_FROM_SOURCE_PNG_INSTEAD_OF_SVG_BECAUSE_OF_B0RKEN_SVG_SUPPORT_IN_IMAGEMAGICK)
		SET(_GMPMPC_ICON_TARGET_NAME "${CMAKE_CURRENT_BINARY_DIR}/gmpmpc_icon_${SIZE}_${BPP}_${COLOR_COUNT}.png")
		ADD_CUSTOM_COMMAND(OUTPUT "${_GMPMPC_ICON_TARGET_NAME}"
					   COMMAND "${IMAGEMAGICK_CONVERT_EXECUTABLE}" -resize "${SIZE}"
										       -depth "${BPP}"
										       -colors "${COLOR_COUNT}"
										       "${_GMPMPC_ICON_SOURCE_NAME}"
										       "${_GMPMPC_ICON_TARGET_NAME}"
					   DEPENDS "${_GMPMPC_ICON_SOURCE_NAME}"
					  )
		SET(GMPMPC_ICON_DEPS ${GMPMPC_ICON_DEPS} "${_GMPMPC_ICON_TARGET_NAME}")
	ENDMACRO(GENERATE_ICON_IMAGE)
	
	UNSET(WIN32_GMPMPC_ICON_RESOURCES)
	UNSET(GMPMPC_ICON_RESOURCE_FILE)
	IF(WIN32)
		UNSET(GMPMPC_ICON_DEPS)
		FOREACH(COLOR_COUNT 0 256 16 65535) # NOTE: -colors 0 == do not use color reduction
			FOREACH(BPP 32 16 8)
				FOREACH(RESOLUTION 32x32 16x16 24x24 48x48)
					GENERATE_ICON_IMAGE(${RESOLUTION} ${BPP} ${COLOR_COUNT})
				ENDFOREACH(RESOLUTION)
			ENDFOREACH(BPP)
		ENDFOREACH(COLOR_COUNT)
		
		ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gmpmpc.ico"
		                   COMMAND "${PNG2ICO_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/gmpmpc.ico"                                                  ${GMPMPC_ICON_DEPS}
		                   DEPENDS ${GMPMPC_ICON_DEPS}
		                  )
		IF(PNG2ICO_EXECUTABLE AND IMAGEMAGICK_CONVERT_EXECUTABLE)
			SET(WIN32_GMPMPC_ICON_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/gmpmpc.ico" "${CMAKE_CURRENT_SOURCE_DIR}/gmpmpc_icon.rc")
		ELSE(PNG2ICO_EXECUTABLE AND IMAGEMAGICK_CONVERT_EXECUTABLE)
			MESSAGE(STATUS "NOTE: In order to build the icons PNG2ICO_EXECUTABLE and IMAGEMAGICK_CONVERT_EXECUTABLE need to be set")
		ENDIF(PNG2ICO_EXECUTABLE AND IMAGEMAGICK_CONVERT_EXECUTABLE)
	ENDIF(WIN32)

	ADD_EXECUTABLE(gmpmpc
	               ${WIN32_GMPMPC_ICON_RESOURCES}
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

	SET(GMPMPC_LINK_LIBRARIES ${GTKMM24_LIBRARIES} ${GTHREAD_LIBRARIES} ${PIXBUF_LIBRARIES})
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

		UNSET(GTKMM_ADDITIONAL_DEPENDENCIES)
		UNSET(GTKMM_ADDITIONAL_INCLUDE_DIRECTORIES)
		UNSET(GTKMM_ADDITIONAL_LIBRARY_DIRECTORIES)
		UNSET(GTKMM24_INCLUDE_DIRS)
		UNSET(GTKMM24_LIBRARY_DIRS)
		UNSET(GTKMM24_LIBRARY_NAMES)
		UNSET(GMPMPC_LINK_LIBRARIES)
		FOREACH(BuildType debug optimized)
			IF(BuildType MATCHES debug)
				SET(_PROPERTY_SHEET ${GTKMM_DEBUG_PROPERTY_SHEET})
			ELSE(BuildType MATCHES debug)
				SET(_PROPERTY_SHEET ${GTKMM_PROPERTY_SHEET})
			ENDIF(BuildType MATCHES debug)

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
			SET(GTKMM24_INCLUDE_DIRS ${GTKMM24_INCLUDE_DIRS} "${GTKMM_ADDITIONAL_s}")
			SET(GTKMM24_LIBRARY_DIRS ${GTKMM24_LIBRARY_DIRS} "${GTKMM_ADDITIONAL_LIBRARY_DIRECTORIES}")

			SEPARATE_ARGUMENTS(GTKMM_ADDITIONAL_DEPENDENCIES)
			SET(GTKMM24_LIBRARY_NAMES ${GTKMM_ADDITIONAL_DEPENDENCIES})

			FOREACH(L ${GTKMM24_LIBRARY_NAMES} ${GTHREAD_LIBRARIES})
				SET(GTK_LIBPATH NOTFOUND)
				FIND_LIBRARY(GTK_LIBPATH
					NAMES ${L}
					PATHS ${GTKMM24_LIBRARY_DIRS} ${GTHREAD_LIBRARY_DIRS}
				)
				SET(GMPMPC_LINK_LIBRARIES ${GMPMPC_LINK_LIBRARIES} ${BuildType} "${GTK_LIBPATH}")
				ENDFOREACH(L)
		ENDFOREACH(BuildType)
	ENDIF(MSVC)

	INCLUDE_DIRECTORIES(${GTKMM24_INCLUDE_DIRS}
	                    ${GTHREAD_INCLUDE_DIRS}
	                    ${PIXBUF_INCLUDE_DIRS}
	                    ${CMAKE_CURRENT_SOURCE_DIR} # For boost/preprocessor/iteration
	                    ${CMAKE_CURRENT_BINARY_DIR} # Make sure embedded file's headers can be found
	                   )

	TARGET_LINK_LIBRARIES(gmpmpc
	                      ${GMPMPC_LINK_LIBRARIES}
	                      ${BOOST_THREAD_LIBRARY}
	                      ${BOOST_SIGNALS_LIBRARY}
	                      ${BOOST_PROGRAM_OPTIONS_LIBRARY}
	                      ${BOOST_SERIALIZATION_LIBRARY}
	                      ${BOOST_FILESYSTEM_LIBRARY}
	                      lib_network
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
		SET_TARGET_PROPERTIES(gmpmpc PROPERTIES COMPILE_FLAGS      "${GTKMM_DISABLE_SPECIFIC_WARNINGS}")
		SET_TARGET_PROPERTIES(gmpmpc PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
	ENDIF(MSVC)

ENDIF(ENABLE_GTK_GUI)
