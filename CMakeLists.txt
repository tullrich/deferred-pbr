cmake_minimum_required( VERSION 2.6 )


# define the path of our additional CMake modules
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" )

project( Deferred )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package( GLM REQUIRED )
find_package( SDL2 REQUIRED )
find_package( OpenGL REQUIRED )
find_package( GLEW REQUIRED )
find_package( AntTweakBar REQUIRED )


add_executable(Deferred 
	src/main.c
	src/utility.h
	src/utility.c
	src/gbuffer.h
	src/gbuffer.c
	src/deferred.h
	src/deferred.c
)

include_directories(
	${GLM_INCLUDE_DIRS}
	${SDL_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${ANT_TWEAK_BAR_INCLUDE_PATH}
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(Deferred
	${SDL_LIBRARY}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${ANT_TWEAK_BAR_LIBRARY}
	"-lm"
)
