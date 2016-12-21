# CMakeLists.txt for STB
cmake_minimum_required( VERSION 2.6 )

SET(STB_INCLUDE_DIR
	${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

include_directories( ${STB_INCLUDE_DIR} )

add_library(stb_lib 
	${CMAKE_CURRENT_SOURCE_DIR}/stb.c)

SET(STB_LIBRARIES stb_lib PARENT_SCOPE)

mark_as_advanced(STB_INCLUDE_DIR)
mark_as_advanced(STB_LIBRARIES)
