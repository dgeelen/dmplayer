FIND_PACKAGE(MyBoost)

INCLUDE_DIRECTORIES("${BOOST_INCLUDE_DIR}")
LINK_DIRECTORIES("${BOOST_LIBRARY_DIR}")

ADD_EXECUTABLE(gmpmpc
	gmpmpc.cpp
)

TARGET_LINK_LIBRARIES(gmpmpc
	${BOOST_THREAD_LIBRARY}
	${BOOST_PROGRAM_OPTIONS_LIBRARY}
)