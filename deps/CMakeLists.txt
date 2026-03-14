cmake_minimum_required(VERSION 3.20)

function(kfx_resolve_dep_archive archive_name archive_url)
	set(archive_dst "${CMAKE_SOURCE_DIR}/deps/${archive_name}")

	if(NOT EXISTS "${archive_dst}" AND DEFINED ENV{KFX_DEPS_CACHE} AND NOT "$ENV{KFX_DEPS_CACHE}" STREQUAL "")
		set(archive_cache_src "$ENV{KFX_DEPS_CACHE}/${archive_name}")
		if(EXISTS "${archive_cache_src}")
			message(STATUS "Using cached dependency archive: ${archive_name}")
			execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${archive_cache_src}" "${archive_dst}")
		endif()
	endif()

	if(NOT EXISTS "${archive_dst}")
		file(DOWNLOAD "${archive_url}" "${archive_dst}" SHOW_PROGRESS)
	endif()

	file(SIZE "${archive_dst}" archive_size)
	if(archive_size EQUAL 0)
		message(FATAL_ERROR "Dependency archive ${archive_name} is empty. URL: ${archive_url}")
	endif()
endfunction()


if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/enet6/lib/libenet6.a )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/enet6)
	kfx_resolve_dep_archive("enet6-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/20260310/enet6-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/enet6-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/enet6)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/zlib/libz.a )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/zlib)
	kfx_resolve_dep_archive("zlib-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/initial/zlib-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/zlib-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/zlib)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/spng/libspng.a )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/spng)
	kfx_resolve_dep_archive("spng-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/initial/spng-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/spng-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/spng)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/astronomy/libastronomy.a )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/astronomy)
	kfx_resolve_dep_archive("astronomy-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/initial/astronomy-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/astronomy-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/astronomy)
endif()

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/centijson/libjson.a )
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/centijson)
	kfx_resolve_dep_archive("centijson-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/initial/centijson-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/centijson-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/centijson)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/ffmpeg/libavcodec/libavcodec.a)
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/ffmpeg)
	kfx_resolve_dep_archive("ffmpeg-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/initial/ffmpeg-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/ffmpeg-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/ffmpeg)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/luajit/lib/libluajit.a)
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/luajit)
	kfx_resolve_dep_archive("luajit-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/20250418/luajit-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/luajit-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/luajit)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/openal/libOpenAL32.a)
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/openal)
	kfx_resolve_dep_archive("openal-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/20260310/openal-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/openal-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/openal)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/miniupnpc/libminiupnpc.a)
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/miniupnpc)
	kfx_resolve_dep_archive("miniupnpc-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/20260102/miniupnpc-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/miniupnpc-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/miniupnpc)
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/deps/libnatpmp/libnatpmp.a)
	file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/libnatpmp)
	kfx_resolve_dep_archive("libnatpmp-mingw32.tar.gz" "https://github.com/dkfans/kfx-deps/releases/download/20260102/libnatpmp-mingw32.tar.gz")

	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/deps/libnatpmp-mingw32.tar.gz WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/libnatpmp)
endif()

## enet
add_library(enet_static STATIC IMPORTED)
set_target_properties(enet_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/enet6/lib/libenet6.a)
set_target_properties(enet_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/enet6/include)
target_link_libraries(enet_static INTERFACE ws2_32 winmm)
target_link_libraries(keeperfx PUBLIC enet_static)
target_link_libraries(keeperfx_hvlog PUBLIC enet_static)

## spng
add_library(spng_static STATIC IMPORTED)
set_target_properties(spng_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/spng/libspng.a)
set_target_properties(spng_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/spng/include)
target_link_libraries(keeperfx PUBLIC spng_static)
target_link_libraries(keeperfx_hvlog PUBLIC spng_static)

## centijson
add_library(centijson_static STATIC IMPORTED)
set_target_properties(centijson_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/centijson/libjson.a)
set_target_properties(centijson_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/centijson/include)
target_link_libraries(keeperfx PUBLIC centijson_static)
target_link_libraries(keeperfx_hvlog PUBLIC centijson_static)

## astronomy.
add_library(astronomy_static STATIC IMPORTED)
set_target_properties(astronomy_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/astronomy/libastronomy.a)
set_target_properties(astronomy_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/astronomy/include)
target_link_libraries(keeperfx PUBLIC astronomy_static)
target_link_libraries(keeperfx_hvlog PUBLIC astronomy_static)

## zlib
add_library(zlib_static STATIC IMPORTED)
set_target_properties(zlib_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/zlib/libz.a)
set_target_properties(zlib_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/zlib/include)
target_link_libraries(keeperfx PUBLIC zlib_static)
target_link_libraries(keeperfx_hvlog PUBLIC zlib_static)

# Add minizip
add_library(minizip_static STATIC IMPORTED)
set_target_properties(minizip_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/zlib/libminizip.a)
set_target_properties(minizip_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/zlib/include)
target_link_libraries(minizip_static INTERFACE zlib_static)
target_link_libraries(keeperfx PUBLIC minizip_static)
target_link_libraries(keeperfx_hvlog PUBLIC minizip_static)

## centitoml.
add_library(centitoml OBJECT "centitoml/toml_api.c")
target_link_libraries(centitoml PUBLIC centijson_static)
target_include_directories(centitoml INTERFACE "centitoml")
target_link_libraries(keeperfx PUBLIC centitoml)
target_link_libraries(keeperfx_hvlog PUBLIC centitoml)

## ffmpeg
add_library(libavcodec_static STATIC IMPORTED)
set_target_properties(libavcodec_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libavcodec/libavcodec.a)
set_target_properties(libavcodec_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

add_library(libavformat_static STATIC IMPORTED)
set_target_properties(libavformat_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libavformat/libavformat.a)
set_target_properties(libavformat_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

add_library(libavutil_static STATIC IMPORTED)
set_target_properties(libavutil_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libavutil/libavutil.a)
set_target_properties(libavutil_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

add_library(libswresample_static STATIC IMPORTED)
set_target_properties(libswresample_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/libswresample/libswresample.a)
set_target_properties(libswresample_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

target_link_libraries(keeperfx PUBLIC libavformat_static libavcodec_static libswresample_static libavutil_static bcrypt)
target_link_libraries(keeperfx_hvlog PUBLIC libavformat_static libavcodec_static libswresample_static libavutil_static bcrypt)

## luajit
add_library(luajit_static STATIC IMPORTED)
set_target_properties(luajit_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/luajit/lib/libluajit.a)
set_target_properties(luajit_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/luajit/include)
target_link_libraries(keeperfx PUBLIC luajit_static)
target_link_libraries(keeperfx_hvlog PUBLIC luajit_static)

## openal
add_library(openal_static STATIC IMPORTED)
set_target_properties(openal_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/openal/libOpenAL32.a)
set_target_properties(openal_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/openal/include)
target_link_libraries(openal_static INTERFACE avrt)
target_link_libraries(keeperfx PUBLIC openal_static)
target_link_libraries(keeperfx_hvlog PUBLIC openal_static)

## miniupnpc
add_library(miniupnpc_static STATIC IMPORTED)
set_target_properties(miniupnpc_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/miniupnpc/libminiupnpc.a)
set_target_properties(miniupnpc_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/miniupnpc/include)
target_link_libraries(keeperfx PUBLIC miniupnpc_static)
target_link_libraries(keeperfx_hvlog PUBLIC miniupnpc_static)

## libnatpmp
add_library(natpmp_static STATIC IMPORTED)
set_target_properties(natpmp_static PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libnatpmp/libnatpmp.a)
set_target_properties(natpmp_static PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/libnatpmp/include)
target_link_libraries(natpmp_static INTERFACE iphlpapi)
target_link_libraries(keeperfx PUBLIC natpmp_static)
target_link_libraries(keeperfx_hvlog PUBLIC natpmp_static)
