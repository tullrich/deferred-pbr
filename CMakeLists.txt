cmake_minimum_required( VERSION 2.6 )

project( Deferred )

find_package( GLM REQUIRED )
find_package( SDL REQUIRED )


add_executable(Deferred 
	src/main.cpp
)

include_directories(
	${GLM_INCLUDE_DIRS}
	${SDL_INCLUDE_DIRS}
)

target_link_libraries(Deferred
	${SDL_LIBRARY}
)
