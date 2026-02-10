##
## CMake toolchain file for PlayStation Vita
##
## This file configures CMake for cross-compilation to PlayStation Vita homebrew.
## It requires VitaSDK to be installed and the VITASDK environment variable to be set.
##
## Usage:
##   cmake -DCMAKE_TOOLCHAIN_FILE=vita.cmake -B build-vita
##

if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "Please define VITASDK environment variable to point to your SDK path!")
endif()

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR armv7)

# VitaSDK paths
set(VITASDK $ENV{VITASDK})
set(CMAKE_C_COMPILER "${VITASDK}/bin/arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "${VITASDK}/bin/arm-vita-eabi-g++")
set(CMAKE_AR "${VITASDK}/bin/arm-vita-eabi-ar")
set(CMAKE_RANLIB "${VITASDK}/bin/arm-vita-eabi-ranlib")

# Target environment
set(CMAKE_FIND_ROOT_PATH ${VITASDK} ${VITASDK}/arm-vita-eabi)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Vita-specific definitions
add_definitions(-DVITA)
add_definitions(-D__VITA__)

# Compiler flags for Vita
set(CMAKE_C_FLAGS_INIT "-Wl,-q -Wall -O3 -ffast-math")
set(CMAKE_CXX_FLAGS_INIT "-Wl,-q -Wall -O3 -ffast-math")

# Vita uses ARM architecture
set(VITA_ARCH_FLAGS "-march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VITA_ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VITA_ARCH_FLAGS}")

# Set the executable suffix for Vita
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# Vita SDK libraries path
link_directories(${VITASDK}/arm-vita-eabi/lib)
include_directories(${VITASDK}/arm-vita-eabi/include)

# Common Vita system libraries
set(VITA_LIBS
    SceLibKernel_stub
    SceDisplay_stub
    SceGxm_stub
    SceSysmodule_stub
    SceCtrl_stub
    ScePgf_stub
    SceCommonDialog_stub
    SceAudio_stub
    SceTouch_stub
    SceNet_stub
    SceNetCtl_stub
    SceHttp_stub
    ScePower_stub
)

message(STATUS "VitaSDK: ${VITASDK}")
message(STATUS "Vita compiler: ${CMAKE_C_COMPILER}")
