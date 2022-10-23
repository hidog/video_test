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

set(ENV{PKG_CONFIG_PATH} ./ffmpeg/lib/pkgconfig)
find_package(PkgConfig)
pkg_search_module(AVFORMAT REQUIRED libavformat)
pkg_search_module(AVUTIL REQUIRED libavutil)
pkg_search_module(AVCODEC REQUIRED libavcodec)

include_directories(${AVFORMAT_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS})
link_directories(${AVFORMAT_LIBRARY_DIRS} ${AVUTIL_LIBRARY_DIRS})

# source 
set( SOURCE main.c 
            media.c )

# head
set( HEAD media.h )

add_executable( ${PROJECT_NAME} ${SOURCE} ${HEAD} )
target_link_libraries( ${PROJECT_NAME} ${AVFORMAT_LIBRARIES} ${AVUTIL_LIBRARIES} ${AVCODEC} )
