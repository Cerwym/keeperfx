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
execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT GIT_BRANCH)
    set(GIT_BRANCH "unknown")
endif()

set(VER_MAJOR       1)
set(VER_MINOR       2)
set(VER_RELEASE     0)
set(VER_BUILD       0)
set(VER_STRING      "${VER_MAJOR}.${VER_MINOR}.${VER_RELEASE}.${VER_BUILD} ${PACKAGE_SUFFIX}")
set(PACKAGE_SUFFIX  "")
set(GIT_REVISION    "${COMMIT_ID}")

# CMAKE_BINARY_DIR is defined in CMakePresets.json.
set(KEEPERFX_VER_DEFS_H_IN ${CMAKE_SOURCE_DIR}/ver_defs.h.in)
set(KEEPERFX_VER_DEFS_H_OUT ${CMAKE_SOURCE_DIR}/src/ver_defs.h)
configure_file(${KEEPERFX_VER_DEFS_H_IN} ${KEEPERFX_VER_DEFS_H_OUT})

# Platform detection - toolchain variables take precedence, explicit flags override autodetect
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    if(VITA OR CMAKE_SYSTEM_NAME STREQUAL "vita")
        set(PLATFORM_VITA ON CACHE BOOL "Build for PlayStation Vita")
    elseif(NINTENDO_3DS OR CMAKE_SYSTEM_NAME STREQUAL "3DS")
        set(PLATFORM_3DS ON CACHE BOOL "Build for Nintendo 3DS")
    elseif(NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
        set(PLATFORM_SWITCH ON CACHE BOOL "Build for Nintendo Switch")
    elseif(DEFINED ENV{VITASDK} AND NOT DEFINED ENV{DEVKITPRO})
        set(PLATFORM_VITA ON CACHE BOOL "Build for PlayStation Vita")
        message(STATUS "Auto-detected VITASDK environment")
    endif()
endif()

if(PLATFORM_VITA)
    message(STATUS "Building for PlayStation Vita")
    add_compile_definitions(PLATFORM_VITA=1)
elseif(PLATFORM_3DS)
    message(STATUS "Building for Nintendo 3DS")
    add_compile_definitions(PLATFORM_3DS=1)
elseif(PLATFORM_SWITCH)
    message(STATUS "Building for Nintendo Switch")
    add_compile_definitions(PLATFORM_SWITCH=1)
else()
    message(STATUS "Building for desktop platform")
endif()

# Dependencies: platform SDKs provide their own; desktop uses vcpkg or find_package
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)
    find_package(SDL2_mixer CONFIG REQUIRED)
    find_package(SDL2_net CONFIG REQUIRED)

    # Optional: OpenGL renderer backend (requires glad via vcpkg)
    option(KEEPERFX_RENDERER_OPENGL "Enable the OpenGL renderer backend" ON)
    if(KEEPERFX_RENDERER_OPENGL)
        find_package(glad CONFIG QUIET)
        if(glad_FOUND)
            message(STATUS "OpenGL renderer backend enabled (glad found)")
        else()
            message(STATUS "OpenGL renderer backend disabled (glad not found; install via vcpkg)")
            set(KEEPERFX_RENDERER_OPENGL OFF)
        endif()
    endif()
else()
    # Homebrew platforms: SDL2 comes from the platform SDK
    message(STATUS "Skipping vcpkg dependency resolution for homebrew platform")
endif()

# Global definitions.
add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)

file(GLOB_RECURSE KEEPERFX_SOURCES_C "src/*.c")
file(GLOB_RECURSE KEEPERFX_SOURCES_CXX "src/*.cpp")

# On homebrew platforms, exclude desktop-only source files
if(PLATFORM_VITA OR PLATFORM_3DS OR PLATFORM_SWITCH)
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/audio/audio_openal\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/input/input_sdl\\.c$")
    # Exclude network modules that depend on SDL_net (not available on homebrew)
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/api\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_tcpsp\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_base_tcp\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_client_tcp\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_server_tcp\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_enet\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_portforward\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_netsession\\.c$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_netsp\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_network\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_network_exchange\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_resync\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_input_lag\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_received_packets\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_redundant_packets\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_checksums\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/net_game\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/bflib_fmvids\\.c$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/KeeperSpeechImp\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/steam_api\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/custom_sprites\\.c$")
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/moonphase\\.c$")
    # Lua: available on Vita via SonicMastr/LuaJIT-Vita (vdpm). Not yet available on 3DS/Switch.
    if(NOT PLATFORM_VITA)
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/lua_api.*\\.c$")
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/lua_base\\.c$")
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/lua_cfg_funcs\\.c$")
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/lua_params\\.c$")
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/lua_triggers\\.c$")
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/lua_utils\\.c$")
        list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/console_cmd\\.c$")
    endif()
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX ".*/scrcapt\\.c$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_sndlib\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_enet\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_base_tcp\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_client_tcp\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_server_tcp\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/bflib_fmvids\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/cdrom\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/linux\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/net_portforward\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/net_resync\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/steam_api\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/renderer/RendererOpenGL\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/platform_gl_sdl2\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/windows\\.cpp$")
    # Exclude desktop-only IPlatform implementations
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/platform/PlatformLinux\\.cpp$")
    list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/platform/PlatformWindows\\.cpp$")
    # Exclude IPlatform implementations for the other homebrew platforms
    if(NOT PLATFORM_VITA)
        list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/platform/PlatformVita\\.cpp$")
    endif()
    if(NOT PLATFORM_3DS)
        list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/platform/Platform3DS\\.cpp$")
    endif()
    if(NOT PLATFORM_SWITCH)
        list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX ".*/platform/PlatformSwitch\\.cpp$")
    endif()
    list(APPEND KEEPERFX_SOURCES_C "src/platform_shims.c")
endif()

# Global definitions for all targets.
add_compile_definitions("DEBUG=$<IF:$<CONFIG:Debug>,1,0>")
add_compile_definitions("SPNG_STATIC=1")
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    add_compile_definitions("SDL_MIXER_AVAILABLE=1")
endif()
# LuaJIT is available on Vita (SonicMastr/LuaJIT-Vita, installed via vdpm).
# 3DS and Switch do not have a LuaJIT port yet.
if(NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    add_compile_definitions("KEEPERFX_LUA_AVAILABLE=1")
endif()

# Build centijson from source for homebrew platforms (prebuilt libjson.a is MinGW32 only)
if(PLATFORM_VITA OR PLATFORM_3DS OR PLATFORM_SWITCH)
    add_library(centijson_static STATIC
        deps/centijson/json.c
        deps/centijson/json-dom.c
        deps/centijson/json-ptr.c
        deps/centijson/value.c)
    target_include_directories(centijson_static PUBLIC deps/centijson/include)
    add_library(centitoml OBJECT deps/centitoml/toml_api.c)
    target_include_directories(centitoml INTERFACE deps/centitoml)
    target_link_libraries(centitoml PUBLIC centijson_static)
endif()

# Add two executable targets: keeperfx and keeperfx_hvlog.
add_executable(keeperfx ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})
target_include_directories(keeperfx PRIVATE src deps/centitoml deps/centijson/include)

target_compile_definitions(keeperfx PUBLIC BFDEBUG_LEVEL=0)
if(WIN32)
    target_sources(keeperfx PRIVATE "res/keeperfx_stdres.rc")
endif()

add_executable(keeperfx_hvlog ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})
target_include_directories(keeperfx_hvlog PRIVATE src deps/centitoml deps/centijson/include)

target_compile_definitions(keeperfx_hvlog PUBLIC BFDEBUG_LEVEL=10)
if(WIN32)
    target_sources(keeperfx_hvlog PRIVATE "res/keeperfx_stdres.rc")
endif()

# Test executable with CUnit — desktop only
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    file(GLOB TEST_SOURCES "tests/*.cpp")
    add_library(cunit_static STATIC
        "deps/CUnit-2.1-3/CUnit/Sources/Basic/Basic.c"
        "deps/CUnit-2.1-3/CUnit/Sources/Framework/TestDB.c"
        "deps/CUnit-2.1-3/CUnit/Sources/Framework/CUError.c"
        "deps/CUnit-2.1-3/CUnit/Sources/Framework/TestRun.c"
        "deps/CUnit-2.1-3/CUnit/Sources/Framework/Util.c"
    )
    target_include_directories(cunit_static PUBLIC "deps/CUnit-2.1-3/CUnit/Headers")

    add_executable(tests ${TEST_SOURCES} ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})
    target_compile_definitions(tests PUBLIC BFDEBUG_LEVEL=0)
    target_link_libraries(tests PRIVATE cunit_static)
endif()

message(STATUS "We are using ${CMAKE_CXX_COMPILER_ID}")

# The default bfd linker in MinGW is extremely slow. LLVM linker (LLD) is much much faster.
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    set_property(TARGET keeperfx PROPERTY LINKER_TYPE LLD)
    set_property(TARGET keeperfx_hvlog PROPERTY LINKER_TYPE LLD)
    set_property(TARGET tests PROPERTY LINKER_TYPE LLD)
endif()

set(WARNFLAGS -Wall -W -Wshadow -Wno-sign-compare -Wno-unused-parameter -Wno-strict-aliasing -Wno-unknown-pragmas -Werror)

if(PLATFORM_VITA OR PLATFORM_3DS OR PLATFORM_SWITCH)
    # Homebrew platforms: ARM/AArch64 cross-compile, no x86 flags, no Windows subsystem
    target_compile_options(keeperfx PRIVATE ${WARNFLAGS} -Wno-format-truncation -Wno-char-subscripts)
    target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS} -Wno-format-truncation -Wno-char-subscripts)
else()
    set(GNU_COMPILER_FLAG -march=x86-64 -fno-omit-frame-pointer -fmessage-length=0)
    set(GNU_LINK_FLAG -mwindows -Wl,--enable-auto-import)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit")
    target_compile_options(keeperfx PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
    target_compile_options(keeperfx_hvlog PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
    target_compile_options(tests PRIVATE ${WARNFLAGS} ${GNU_COMPILER_FLAG})
    target_link_options(keeperfx PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx.map)
    target_link_options(keeperfx_hvlog PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,keeperfx_hvlog.map)
    target_link_options(tests PRIVATE ${GNU_LINK_FLAG} -Wl,-Map,tests.map)
    target_link_libraries(keeperfx PUBLIC -static stdc++ winpthread -dynamic)
    target_link_libraries(keeperfx_hvlog PUBLIC -static stdc++ winpthread -dynamic)
    target_link_libraries(tests PUBLIC -static stdc++ winpthread -dynamic)
endif()

# System libraries (Windows only).
if(WIN32)
    target_link_libraries(keeperfx PRIVATE imagehlp dbghelp ole32 uuid winmm ws2_32)
    target_link_libraries(keeperfx_hvlog PRIVATE imagehlp dbghelp ole32 uuid winmm ws2_32)
    target_link_libraries(tests PRIVATE imagehlp dbghelp ole32 uuid winmm ws2_32)
endif()

# Go into submodules (desktop only — homebrew platforms use their own SDK deps).
if(NOT PLATFORM_VITA AND NOT PLATFORM_3DS AND NOT PLATFORM_SWITCH)
    add_subdirectory(deps)
    add_subdirectory(tools)
endif()

# External libraries.
if(PLATFORM_VITA)
    # SDL2 cmake config is installed under the vitasdk sysroot
    list(APPEND CMAKE_PREFIX_PATH "$ENV{VITASDK}/arm-vita-eabi")
    find_package(SDL2 REQUIRED)
    target_link_libraries(keeperfx PRIVATE SDL2::SDL2)
    target_link_libraries(keeperfx_hvlog PRIVATE SDL2::SDL2)
    # vitaGL is optional — install via vdpm when ready
    find_library(VITAGL_LIB vitaGL HINTS "$ENV{VITASDK}/arm-vita-eabi/lib")
    if(VITAGL_LIB)
        message(STATUS "vitaGL found: ${VITAGL_LIB}")
        target_link_libraries(keeperfx PRIVATE ${VITAGL_LIB})
        target_link_libraries(keeperfx_hvlog PRIVATE ${VITAGL_LIB})
    else()
        message(STATUS "vitaGL not found — GPU renderer will be a no-op stub. Install via: sudo -E /tmp/vdpm/vdpm vitaGL")
    endif()
    target_link_libraries(keeperfx PRIVATE SceGxm_stub SceDisplay_stub SceCtrl_stub SceTouch_stub ScePower_stub SceAudio_stub)
    target_link_libraries(keeperfx_hvlog PRIVATE SceGxm_stub SceDisplay_stub SceCtrl_stub SceTouch_stub ScePower_stub SceAudio_stub)
    target_link_libraries(keeperfx PRIVATE centijson_static centitoml)
    target_link_libraries(keeperfx_hvlog PRIVATE centijson_static centitoml)
    # LuaJIT for the Vita — cross-compiled via SonicMastr/LuaJIT-Vita fork.
    # Install prerequisites: vdpm luajit taihen vita-rss-libdl
    #
    # WHY taihen is required at runtime:
    #   LuaJIT's JIT compiler must allocate read-write-execute memory pages.  The
    #   Vita OS normally disallows user code from creating executable mappings.
    #   taiHEN is a kernel plugin (part of HENkaku / any homebrew-enabled Vita) that
    #   patches those restrictions.  Any Vita running homebrew already has taiHEN
    #   loaded (it is a prerequisite of every homebrew launcher), so end users do not
    #   need to install anything extra.  The taihen_stub.a at link time simply
    #   generates the SVC stubs to call into the kernel plugin at runtime.
    #   vita-rss-libdl provides the dlopen/dlsym shim that LuaJIT uses for C FFI.
    find_library(LUAJIT_LIB luajit-5.1 HINTS "$ENV{VITASDK}/arm-vita-eabi/lib" REQUIRED)
    find_path(LUAJIT_INCLUDE luajit.h HINTS "$ENV{VITASDK}/arm-vita-eabi/include/luajit-2.1" REQUIRED)
    message(STATUS "LuaJIT found: ${LUAJIT_LIB}")
    target_include_directories(keeperfx PRIVATE ${LUAJIT_INCLUDE})
    target_include_directories(keeperfx_hvlog PRIVATE ${LUAJIT_INCLUDE})
    target_link_libraries(keeperfx PRIVATE -Wl,--start-group ${LUAJIT_LIB} taihen_stub taihenModuleUtils_stub SceSblSsMgr_stub dl -Wl,--end-group)
    target_link_libraries(keeperfx_hvlog PRIVATE -Wl,--start-group ${LUAJIT_LIB} taihen_stub taihenModuleUtils_stub SceSblSsMgr_stub dl -Wl,--end-group)

    # VPK packaging — requires: cmake --build --target keeperfx.vpk
    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
    set(VITA_APP_NAME "KeeperFX")
    set(VITA_TITLEID  "KFXV00001")
    set(VITA_VERSION  "01.00")
    # UNSAFE needed because LuaJIT requires taiHEN RWX memory (standard on HENkaku consoles)
    vita_create_self(keeperfx.self keeperfx UNSAFE)
    vita_create_vpk(keeperfx.vpk ${VITA_TITLEID} keeperfx.self
        VERSION ${VITA_VERSION}
        NAME    ${VITA_APP_NAME}
    )
elseif(PLATFORM_3DS)
    target_link_libraries(keeperfx PRIVATE citro3d ctru)
    target_link_libraries(keeperfx_hvlog PRIVATE citro3d ctru)
elseif(PLATFORM_SWITCH)
    target_link_libraries(keeperfx PRIVATE nx)
    target_link_libraries(keeperfx_hvlog PRIVATE nx)
else()
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

    # Link test executable with SDL2 libraries
    target_link_libraries(tests
        PRIVATE
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
    target_link_libraries(tests
        PRIVATE $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
    target_link_libraries(tests
        PRIVATE $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>)
    target_link_libraries(tests
        PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
endif()

# OpenGL renderer backend
if(KEEPERFX_RENDERER_OPENGL)
    foreach(target keeperfx keeperfx_hvlog tests)
        target_compile_definitions(${target} PRIVATE RENDERER_OPENGL_ENABLED=1)
        target_link_libraries(${target} PRIVATE glad::glad opengl32)
    endforeach()
endif()
