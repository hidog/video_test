# Project Name
set( PROJECT_NAME video_test )
project(${PROJECT_NAME})

if( "${CMAKE_BUILD_TYPE}" STREQUAL "") 
	set(CMAKE_BUILD_TYPE Release)
endif()

# preprocessor define
set( DEFINE ${DEFINE} -DUNIX )
add_definitions( ${DEFINE} )

# include path 
set( INCLUDE_PATH ${CMAKE_SOURCE_DIR}/ffmpeg/include )
include_directories( ${INCLUDE_PATH} )

# library path
set( LIB_PATH ${CMAKE_SOURCE_DIR}/ffmpeg/lib/ )
link_directories( ${LIB_PATH} )

# source 
set( SOURCE main.c 
			media.c )

# head
set( HEAD media.h )

add_executable( ${PROJECT_NAME} ${SOURCE} ${HEAD} )
target_link_libraries( ${PROJECT_NAME} pthread avcodec avdevice avfilter postproc swresample swscale avformat avutil )
