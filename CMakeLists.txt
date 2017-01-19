cmake_minimum_required( VERSION 2.6 )

# define the path of our additional CMake modules
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" )

project( Deferred )

find_package( SDL2 REQUIRED )
find_package( OpenGL REQUIRED )
find_package( GLEW REQUIRED )
find_package( AntTweakBar REQUIRED )

add_subdirectory( thirdparty/stb )

add_executable( Deferred
	src/main.c
	src/common.h
	src/utility.h
	src/utility.c
	src/gbuffer.h
	src/gbuffer.c
	src/deferred.h
	src/deferred.c
	src/forward.h
	src/forward.c
	src/particles.h
	src/particles.c
	src/scene.h
	src/scene.c
)

if ( NOT MSVC )
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
	target_compile_options( Deferred PUBLIC -g )
	target_link_libraries( Deferred "-lm" )
endif()

target_compile_definitions( Deferred PUBLIC -DTW_STATIC -DTW_NO_LIB_PRAGMA)

include_directories(
	${GLM_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${ANT_TWEAK_BAR_INCLUDE_PATH}
	${STB_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(Deferred
	${SDL2_LIBRARY}
	${SDL2MAIN_LIBRARY}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${ANT_TWEAK_BAR_LIBRARY}
	${STB_LIBRARIES}
)
