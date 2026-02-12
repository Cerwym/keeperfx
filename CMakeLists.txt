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

# Platform detection - uses toolchain-provided variables when available
# Falls back to environment variable detection
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    # Check for toolchain-defined variables first (more reliable)
    if(VITA)
        set(PLATFORM_VITA ON CACHE BOOL "Build for PlayStation Vita")
    elseif(NINTENDO_3DS OR CMAKE_SYSTEM_NAME STREQUAL "3DS")
        set(PLATFORM_3DS ON CACHE BOOL "Build for Nintendo 3DS")
    elseif(NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
        set(PLATFORM_SWITCH ON CACHE BOOL "Build for Nintendo Switch")
    else()
        # Fall back to environment variables
        set(SDK_COUNT 0)
        
        if(DEFINED ENV{VITASDK})
            math(EXPR SDK_COUNT "${SDK_COUNT} + 1")
            set(PLATFORM_VITA ON CACHE BOOL "Build for PlayStation Vita")
            message(STATUS "Detected VITASDK environment")
        endif()

        if(DEFINED ENV{DEVKITPRO})
            if(DEFINED ENV{DEVKITARM})
                math(EXPR SDK_COUNT "${SDK_COUNT} + 1")
                set(PLATFORM_3DS ON CACHE BOOL "Build for Nintendo 3DS")
                message(STATUS "Detected DEVKITARM environment")
            endif()
            if(DEFINED ENV{DEVKITA64})
                math(EXPR SDK_COUNT "${SDK_COUNT} + 1")
                set(PLATFORM_SWITCH ON CACHE BOOL "Build for Nintendo Switch")
                message(STATUS "Detected DEVKITA64 environment")
            endif()
        endif()
        
        # Warn if multiple SDKs detected
        if(SDK_COUNT GREATER 1)
            message(WARNING "Multiple homebrew SDK environments detected. "
                    "Please explicitly specify target platform with -DPLATFORM_VITA=ON, "
                    "-DPLATFORM_3DS=ON, or -DPLATFORM_SWITCH=ON to avoid ambiguity.")
        endif()
    endif()
endif()

# Report selected platform
if(PLATFORM_VITA)
    message(STATUS "Building for PlayStation Vita")
elseif(PLATFORM_3DS)
    message(STATUS "Building for Nintendo 3DS")
elseif(PLATFORM_SWITCH)
    message(STATUS "Building for Nintendo Switch")
else()
    message(STATUS "Building for desktop platform")
endif()

# Dependency management option
option(DOWNLOAD_DEPENDENCIES "Download and build dependencies using FetchContent" ON)

# Find or fetch dependencies based on platform
if(PLATFORM_VITA)
    # Vita libraries come from vitasdk but must be explicitly linked
    # vitaGL provides OpenGL ES wrapper over GXM for bgfx compatibility
    message(STATUS "Using Vita SDK libraries (vitaGL for OpenGL ES)")
elseif(PLATFORM_3DS OR PLATFORM_SWITCH)
    # 3DS and Switch libraries come from devkitPro
    message(STATUS "Using devkitPro libraries")
else()
    # Desktop platforms: use FetchContent or find_package
    if(DOWNLOAD_DEPENDENCIES)
        message(STATUS "Downloading dependencies via FetchContent...")
        include(FetchContent)
        
        # SDL2
        message(STATUS "Fetching SDL2...")
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.30.9
            GIT_SHALLOW TRUE
        )
        set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
        
        # SDL2_image
        message(STATUS "Fetching SDL2_image...")
        FetchContent_Declare(
            SDL2_image
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
            GIT_TAG release-2.8.2
            GIT_SHALLOW TRUE
        )
        set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2_image)
        
        # SDL2_mixer
        message(STATUS "Fetching SDL2_mixer...")
        FetchContent_Declare(
            SDL2_mixer
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
            GIT_TAG release-2.8.0
            GIT_SHALLOW TRUE
        )
        set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2_mixer)
        
        # SDL2_net
        message(STATUS "Fetching SDL2_net...")
        FetchContent_Declare(
            SDL2_net
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
            GIT_TAG release-2.2.0
            GIT_SHALLOW TRUE
        )
        set(SDL2NET_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2_net)
        
        # bgfx (requires bx and bimg)
        # Note: bgfx doesn't use semantic versioning, so we pin to specific commits
        message(STATUS "Fetching bgfx and dependencies...")
        FetchContent_Declare(
            bx
            GIT_REPOSITORY https://github.com/bkaradzic/bx.git
            GIT_TAG 4db4369f66a00a2a7b9eea39e338d2e8e5c62543  # Latest stable as of 2024-12
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(bx)
        
        FetchContent_Declare(
            bimg
            GIT_REPOSITORY https://github.com/bkaradzic/bimg.git
            GIT_TAG 847863a2fa30b9c49d58f89e34b8912ccbb59b13  # Latest stable as of 2024-12
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(bimg)
        
        FetchContent_Declare(
            bgfx
            GIT_REPOSITORY https://github.com/bkaradzic/bgfx.git
            GIT_TAG 1e8c9f5b2ed95b92bf8e634ce4adf8e58f02e8c4  # Latest stable as of 2024-12
            GIT_SHALLOW TRUE
        )
        set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
        set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(bgfx)
        
        # Create aliases for compatibility
        if(NOT TARGET SDL2::SDL2)
            add_library(SDL2::SDL2 ALIAS SDL2)
        endif()
        if(NOT TARGET SDL2_image::SDL2_image)
            add_library(SDL2_image::SDL2_image ALIAS SDL2_image)
        endif()
        if(NOT TARGET SDL2_mixer::SDL2_mixer)
            add_library(SDL2_mixer::SDL2_mixer ALIAS SDL2_mixer)
        endif()
        if(NOT TARGET SDL2_net::SDL2_net)
            add_library(SDL2_net::SDL2_net ALIAS SDL2_net)
        endif()
        
        message(STATUS "Dependencies fetched successfully")
    else()
        # Use system-installed packages via find_package
        message(STATUS "Using system-installed dependencies...")
        find_package(SDL2 CONFIG REQUIRED)
        find_package(SDL2_image CONFIG REQUIRED)
        find_package(SDL2_mixer CONFIG REQUIRED)
        find_package(SDL2_net CONFIG REQUIRED)
        find_package(bgfx CONFIG REQUIRED)
    endif()
endif()

# Global definitions.
add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)

file(GLOB_RECURSE KEEPERFX_SOURCES_C "src/*.c")
file(GLOB_RECURSE KEEPERFX_SOURCES_CXX "src/*.cpp")

# Global definitions for all targets.
add_compile_definitions("DEBUG=$<IF:$<CONFIG:Debug>,1,0>")
add_compile_definitions("SPNG_STATIC=1")

# Add two executable targets: keeperfx and keeperfx_hvlog.
add_executable(keeperfx ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})

target_compile_definitions(keeperfx PUBLIC BFDEBUG_LEVEL=0)
target_sources(keeperfx PRIVATE "res/keeperfx_stdres.rc")

add_executable(keeperfx_hvlog ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})

target_compile_definitions(keeperfx_hvlog PUBLIC BFDEBUG_LEVEL=10)
target_sources(keeperfx_hvlog PRIVATE "res/keeperfx_stdres.rc")

message(STATUS "We are using ${CMAKE_CXX_COMPILER_ID}")

# The default bfd linker in MinGW is extremely slow. LLVM linker (LLD) is much much faster.
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    set_property(TARGET keeperfx PROPERTY LINKER_TYPE LLD)
    set_property(TARGET keeperfx_hvlog PROPERTY LINKER_TYPE LLD)
endif()

set(WARNFLAGS -Wall -W -Wshadow -Wno-sign-compare -Wno-unused-parameter -Wno-strict-aliasing -Wno-unknown-pragmas -Werror)

# Platform-specific compiler flags
if(PLATFORM_VITA OR PLATFORM_3DS OR PLATFORM_SWITCH)
    # ARM platforms: architecture flags are set by platform-specific toolchain files
    # (ARMv6 for 3DS, ARMv7 for Vita, ARMv8/AArch64 for Switch)
    set(GNU_COMPILER_FLAG -fno-omit-frame-pointer -fmessage-length=0)
else()
    # x86-64 for desktop platforms
    set(GNU_COMPILER_FLAG -march=x86-64 -fno-omit-frame-pointer -fmessage-length=0)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit")
target_compile_options(keeperfx PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})

# System libraries (Windows only).
if(WIN32)
    set(GNU_LINK_FLAG -mwindows -Wl,--enable-auto-import)
    target_link_options(keeperfx PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx.map)
    target_link_options(keeperfx_hvlog PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx_hvlog.map)
    target_link_libraries(keeperfx PRIVATE imagehlp dbghelp)
    target_link_libraries(keeperfx_hvlog PRIVATE imagehlp dbghelp)
    target_link_libraries(keeperfx PUBLIC -static stdc++ winpthread -dynamic)
    target_link_libraries(keeperfx_hvlog PUBLIC -static stdc++ winpthread -dynamic)
endif()

# Go into submodules.
add_subdirectory(deps)
add_subdirectory(tools)

# External libraries.
if(PLATFORM_VITA)
    # Vita-specific libraries (vitaGL, SceLibc, etc.)
    # Note: keeperfx_hvlog is not typically used on Vita due to memory constraints
    target_link_libraries(keeperfx PRIVATE
        SDL2
        vitaGL
        vitashark
        SceLibKernel_stub
        SceDisplay_stub
        SceGxm_stub
        SceSysmodule_stub
        SceCtrl_stub
        SceTouch_stub
        SceMotion_stub
        m
        SceAudio_stub
        SceCommonDialog_stub
    )
elseif(PLATFORM_3DS)
    # 3DS-specific libraries (citro3d, etc.)
    # Note: keeperfx_hvlog is not typically used on 3DS due to memory constraints
    target_link_libraries(keeperfx PRIVATE
        citro3d
        ctru
        m
    )
elseif(PLATFORM_SWITCH)
    # Switch-specific libraries
    # Note: keeperfx_hvlog is not typically used on Switch for homebrew builds
    target_link_libraries(keeperfx PRIVATE
        SDL2
        EGL
        glapi
        drm_nouveau
        nx
        m
    )
else()
    # Desktop platforms - link for both keeperfx and keeperfx_hvlog
    if(DOWNLOAD_DEPENDENCIES)
        # FetchContent targets
        target_link_libraries(keeperfx PRIVATE SDL2::SDL2)
        target_link_libraries(keeperfx PRIVATE SDL2_mixer::SDL2_mixer)
        target_link_libraries(keeperfx PRIVATE SDL2_net::SDL2_net)
        target_link_libraries(keeperfx PRIVATE SDL2_image::SDL2_image)
        target_link_libraries(keeperfx PRIVATE bgfx bx bimg)

        target_link_libraries(keeperfx_hvlog PRIVATE SDL2::SDL2)
        target_link_libraries(keeperfx_hvlog PRIVATE SDL2_mixer::SDL2_mixer)
        target_link_libraries(keeperfx_hvlog PRIVATE SDL2_net::SDL2_net)
        target_link_libraries(keeperfx_hvlog PRIVATE SDL2_image::SDL2_image)
        target_link_libraries(keeperfx_hvlog PRIVATE bgfx bx bimg)
    else()
        # System packages
        target_link_libraries(keeperfx
            PRIVATE
            $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
        target_link_libraries(keeperfx
            PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
        target_link_libraries(keeperfx
            PRIVATE $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
        target_link_libraries(keeperfx
            PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)

        # bgfx library
        target_link_libraries(keeperfx PRIVATE bgfx::bgfx bx::bx bimg::bimg)

        target_link_libraries(keeperfx_hvlog
            PRIVATE
            $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
        target_link_libraries(keeperfx_hvlog
            PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
        target_link_libraries(keeperfx_hvlog
            PRIVATE $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
        target_link_libraries(keeperfx_hvlog
            PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)

        # bgfx library
        target_link_libraries(keeperfx_hvlog PRIVATE bgfx::bgfx bx::bx bimg::bimg)
    endif()
endif()

# Testing support with CUnit
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
    enable_testing()
    
    # Add CUnit library if it exists
    if(EXISTS ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3)
        add_library(cunit_static STATIC IMPORTED)
        
        # Try to find the CUnit library
        if(EXISTS ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3/lib/libcunit.a)
            set_target_properties(cunit_static PROPERTIES 
                IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3/lib/libcunit.a)
        elseif(EXISTS ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3/CUnit/Sources/.libs/libcunit.a)
            set_target_properties(cunit_static PROPERTIES 
                IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3/CUnit/Sources/.libs/libcunit.a)
        endif()
        
        # Set include directories
        if(EXISTS ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3/CUnit/Headers)
            set_target_properties(cunit_static PROPERTIES 
                INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/deps/CUnit-2.1-3/CUnit/Headers)
        endif()
        
        # Build test executable
        file(GLOB TEST_SOURCES "tests/*.cpp" "tests/*.c")
        if(TEST_SOURCES)
            # Note: Currently includes all source files - this may need refactoring
            # to link against main targets as libraries or manage entry points carefully
            add_executable(keeperfx_tests ${TEST_SOURCES} ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})
            
            # Copy compile options from main target
            target_compile_options(keeperfx_tests PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
            target_compile_definitions(keeperfx_tests PUBLIC BFDEBUG_LEVEL=0)
            target_include_directories(keeperfx_tests PRIVATE src/)
            
            # Link libraries
            target_link_libraries(keeperfx_tests PRIVATE cunit_static)
            target_link_libraries(keeperfx_tests PRIVATE
                $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
            target_link_libraries(keeperfx_tests PRIVATE
                $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
            target_link_libraries(keeperfx_tests PRIVATE
                $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
            target_link_libraries(keeperfx_tests PRIVATE
                $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
            target_link_libraries(keeperfx_tests PRIVATE bgfx::bgfx bx::bx bimg::bimg)
            
            # Windows-specific libraries
            if(WIN32)
                target_link_libraries(keeperfx_tests PRIVATE imagehlp dbghelp)
                target_link_libraries(keeperfx_tests PUBLIC -static stdc++ winpthread -dynamic)
            endif()
            
            # Add the test
            add_test(NAME keeperfx_unit_tests COMMAND keeperfx_tests)
        endif()
    endif()
endif()
