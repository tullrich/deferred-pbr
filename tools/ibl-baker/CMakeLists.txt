find_package( FreeImage REQUIRED )

add_executable( IBLBaker
	main.cpp
	ibl-baker.h
	ibl-baker.cpp
	ibl.h
	ibl.cpp
	cubemap.h
	cubemap.cpp
)

if ( NOT MSVC )
	target_compile_options( IBLBaker PUBLIC -g )
	target_link_libraries( IBLBaker "-lm" )
else()
	target_compile_definitions( IBLBaker PUBLIC -D_CRT_SECURE_NO_WARNINGS)
endif()

target_compile_definitions( IBLBaker PUBLIC -DTW_STATIC -DTW_NO_LIB_PRAGMA)

include_directories(
	${STB_INCLUDE_DIR}
	${FREEIMAGE_INCLUDE_PATH}
	${GLI_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/thirdparty
)

target_link_libraries(IBLBaker
	${STB_LIBRARIES}
	${FREEIMAGE_LIBRARIES}
	${GLI_LIBRARY_DIR}
)
