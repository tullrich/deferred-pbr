cmake_minimum_required( VERSION 2.6 )


# define the path of our additional CMake modules
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" )

project( Deferred )

find_package( GLM REQUIRED )
find_package( SDL2 REQUIRED )
find_package( OpenGL REQUIRED )
find_package( GLEW REQUIRED )


add_executable(Deferred 
	src/main.cpp
)

include_directories(
	${GLM_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
)

target_link_libraries(Deferred
	${SDL2_LIBRARY}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
)
