cmake_minimum_required(VERSION 3.20)

# Do not allow building in root
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Do not build in the root. Create a bin directory and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif ()

project(keeperfx C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Get the abbreviated commit Id of the head.
find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" describe --always OUTPUT_VARIABLE COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)

set(VER_MAJOR       1)
set(VER_MINOR       2)
set(VER_RELEASE     0)
set(VER_BUILD       0)
set(VER_STRING      "${VER_MAJOR}.${VER_MINOR}.${VER_RELEASE}.${VER_BUILD} ${PACKAGE_SUFFIX}")
set(PACKAGE_SUFFIX  "")
set(GIT_REVISION    "${COMMIT_ID}")

# CMAKE_BINARY_DIR is defined in CMakePresets.json.
set(KEEPERFX_VER_DEFS_H_IN ${CMAKE_SOURCE_DIR}/ver_defs.h.in)
set(KEEPERFX_VER_DEFS_H_OUT ${CMAKE_SOURCE_DIR}/ver_defs.h)
configure_file(${KEEPERFX_VER_DEFS_H_IN} ${KEEPERFX_VER_DEFS_H_OUT})

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_net CONFIG REQUIRED)

# Global definitions.
add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)

file(GLOB_RECURSE KEEPERFX_SOURCES_C "src/*.c")
file(GLOB_RECURSE KEEPERFX_SOURCES_CXX "src/*.cpp")

# Platform-specific source filtering
if(VITA)
    # Exclude Windows-specific files
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*windows\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*linux\\.cpp$")
elseif(WIN32)
    # Exclude non-Windows files
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*linux\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*vita\\.cpp$")
else()
    # Linux - exclude Windows and Vita files
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*windows\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*vita\\.cpp$")
endif()

# Global definitions for all targets.
add_compile_definitions("DEBUG=$<IF:$<CONFIG:Debug>,1,0>")
add_compile_definitions("SPNG_STATIC=1")

# Add two executable targets: keeperfx and keeperfx_hvlog.
add_executable(keeperfx ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})

target_compile_definitions(keeperfx PUBLIC BFDEBUG_LEVEL=0)

# Platform-specific resources
if(WIN32 AND NOT VITA)
    target_sources(keeperfx PRIVATE "res/keeperfx_stdres.rc")
endif()

add_executable(keeperfx_hvlog ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})

target_compile_definitions(keeperfx_hvlog PUBLIC BFDEBUG_LEVEL=10)

# Platform-specific resources
if(WIN32 AND NOT VITA)
    target_sources(keeperfx_hvlog PRIVATE "res/keeperfx_stdres.rc")
endif()

message(STATUS "We are using ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}, Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Platform detection - VITA: ${VITA}, WIN32: ${WIN32}, UNIX: ${UNIX}")

# Platform-specific compiler and linker flags
if(VITA)
    # Vita-specific flags (ARM architecture)
    message(STATUS "Configuring for PlayStation Vita (ARM)")
    set(WARNFLAGS -Wall -W -Wshadow -Wno-sign-compare -Wno-unused-parameter -Wno-strict-aliasing -Wno-unknown-pragmas)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit")
    target_compile_options(keeperfx PRIVATE ${WARNFLAGS})
    target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS})
    
    # Vita system libraries will be linked via toolchain
elseif(WIN32)
    # Windows/MinGW flags (x86-64 architecture)
    message(STATUS "Configuring for Windows (x86-64)")
    # The default bfd linker in MinGW is extremely slow. LLVM linker (LLD) is much much faster.
    set_property(TARGET keeperfx PROPERTY LINKER_TYPE LLD)
    set_property(TARGET keeperfx_hvlog PROPERTY LINKER_TYPE LLD)
    
    set(WARNFLAGS -Wall -W -Wshadow -Wno-sign-compare -Wno-unused-parameter -Wno-strict-aliasing -Wno-unknown-pragmas -Werror)
    set(GNU_COMPILER_FLAG -march=x86-64 -fno-omit-frame-pointer -fmessage-length=0)
    set(GNU_LINK_FLAG -mwindows -Wl,--enable-auto-import)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit")
    target_compile_options(keeperfx PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
    target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
    target_link_options(keeperfx PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx.map)
    target_link_options(keeperfx_hvlog PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx_hvlog.map)
    target_link_libraries (keeperfx PUBLIC -static stdc++ winpthread -dynamic)
    target_link_libraries (keeperfx_hvlog PUBLIC -static stdc++ winpthread -dynamic)
    
    # System libraries.
    target_link_libraries(keeperfx PRIVATE imagehlp dbghelp)
    target_link_libraries(keeperfx_hvlog PRIVATE imagehlp dbghelp)
else()
    # Linux/other platforms (x86-64 architecture)
    message(STATUS "Configuring for Linux/Unix (x86-64)")
    set(WARNFLAGS -Wall -W -Wshadow -Wno-sign-compare -Wno-unused-parameter -Wno-strict-aliasing -Wno-unknown-pragmas -Werror)
    set(GNU_COMPILER_FLAG -march=x86-64 -fno-omit-frame-pointer -fmessage-length=0)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit")
    target_compile_options(keeperfx PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
    target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
endif()

# Go into submodules.
add_subdirectory(deps)
add_subdirectory(tools)

# External libraries.
target_link_libraries(keeperfx
    PRIVATE
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(keeperfx
    PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
target_link_libraries(keeperfx
    PRIVATE $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
target_link_libraries(keeperfx
    PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)

target_link_libraries(keeperfx_hvlog
    PRIVATE
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
target_link_libraries(keeperfx_hvlog
    PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
target_link_libraries(keeperfx_hvlog
    PRIVATE $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
target_link_libraries(keeperfx_hvlog
    PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)

# Vita-specific libraries
if(VITA)
    target_link_libraries(keeperfx PRIVATE
        SceLibKernel_stub
        SceDisplay_stub
        SceGxm_stub
        SceSysmodule_stub
        SceCtrl_stub
        ScePgf_stub
        SceCommonDialog_stub
        SceAudio_stub
        SceTouch_stub
        m
        stdc++
    )
    
    target_link_libraries(keeperfx_hvlog PRIVATE
        SceLibKernel_stub
        SceDisplay_stub
        SceGxm_stub
        SceSysmodule_stub
        SceCtrl_stub
        ScePgf_stub
        SceCommonDialog_stub
        SceAudio_stub
        SceTouch_stub
        m
        stdc++
    )
endif()
