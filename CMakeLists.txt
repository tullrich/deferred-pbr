cmake_minimum_required( VERSION 2.6 )

# define the path of our additional CMake modules
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" )

project( Deferred )

find_package( SDL2 REQUIRED )
find_package( OpenGL REQUIRED )
find_package( GLEW REQUIRED )
find_package( FreeImage REQUIRED )

add_subdirectory( thirdparty/stb )

if ( NOT MSVC )
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/")
endif()

add_executable( Deferred
	src/main.cpp
	src/common.h
	src/utility.h
	src/utility.cpp
	src/gbuffer.h
	src/gbuffer.cpp
	src/deferred.h
	src/deferred.cpp
	src/forward.h
	src/forward.cpp
	src/particles.h
	src/particles.cpp
	src/scene.h
	src/scene.cpp
	src/ibl.h
	src/ibl.cpp

	src/imgui/imconfig.h
	src/imgui/imgui.h
	src/imgui/imgui_internal.h
	src/imgui/imgui_demo.cpp
	src/imgui/imgui_draw.cpp
	src/imgui/imgui.cpp
	src/imgui/stb_textedit.h
	src/imgui/imgui_impl_sdl_gl3.h
	src/imgui/imgui_impl_sdl_gl3.cpp

	src/tinyobj_loader_c/tinyobj_loader_c.c
)

if ( NOT MSVC )
	target_compile_options( Deferred PUBLIC -g )
	target_link_libraries( Deferred "-lm" )
else()
	target_compile_definitions( Deferred PUBLIC -D_CRT_SECURE_NO_WARNINGS)
endif()

target_compile_definitions( Deferred PUBLIC -DTW_STATIC -DTW_NO_LIB_PRAGMA)

include_directories(
	${GLM_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${STB_INCLUDE_DIR}
	${FREEIMAGE_INCLUDE_PATH}
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(Deferred
	${SDL2_LIBRARY}
	${SDL2MAIN_LIBRARY}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${STB_LIBRARIES}
	${FREEIMAGE_LIBRARIES}
)
