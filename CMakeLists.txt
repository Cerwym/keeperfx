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
find_package(bgfx CONFIG REQUIRED)

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
set_property(TARGET keeperfx PROPERTY LINKER_TYPE LLD)
set_property(TARGET keeperfx_hvlog PROPERTY LINKER_TYPE LLD)

set(WARNFLAGS -Wall -W -Wshadow -Wno-sign-compare -Wno-unused-parameter -Wno-strict-aliasing -Wno-unknown-pragmas -Werror)
set(GNU_COMPILER_FLAG -march=x86-64 -fno-omit-frame-pointer -fmessage-length=0)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit")
target_compile_options(keeperfx PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})

# System libraries (Windows only).
if(WIN32)
    target_link_libraries(keeperfx PRIVATE imagehlp dbghelp)
    target_link_libraries(keeperfx_hvlog PRIVATE imagehlp dbghelp)
    target_link_libraries(keeperfx PUBLIC -static stdc++ winpthread -dynamic)
    target_link_libraries(keeperfx_hvlog PUBLIC -static stdc++ winpthread -dynamic)
    target_link_options(keeperfx PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx.map)
    target_link_options(keeperfx_hvlog PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx_hvlog.map)
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
