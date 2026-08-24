cmake_minimum_required(VERSION 3.10)

if (NOT DEFINED ENV{VITASDK} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "Please define VITASDK env var to point to your SDK path or set CMAKE_TOOLCHAIN_FILE.")
endif ()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Toolchain file")

project(advena C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

# Vita application configuration
set(VITA_APP_NAME "Advena")
set(VITA_TITLEID "ADVENA001")
set(VITA_VPKNAME "advena")
set(VITA_VERSION "01.01")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

# Optional: used for "send" and "dump" targets
set(PSVITAIP "192.168.3.15" CACHE STRING "PSVita IP (for FTP access)")
set(PARSECOREPATH "$ENV{PARSECOREPATH}" CACHE STRING "vita-parse-core main.py")

# Game-specific definitions
set(DATA_PATH "ux0:data/advena/" CACHE STRING "Path to data (with trailing /)")
set(SO_PATH "${DATA_PATH}libgameDSO.so" CACHE STRING "Path to .so")
add_definitions(-DDATA_PATH="${DATA_PATH}" -DSO_PATH="${SO_PATH}")

option(USE_SCELIBC_IO "Use SceLibcBridge for IO and some other std functions" ON)
if (USE_SCELIBC_IO)
  add_definitions(-DUSE_SCELIBC_IO)
endif()

set(SHADER_FORMAT "GLSL" CACHE STRING "Preferred shader format (one of 'GLSL', 'CG', 'GXP')")
if (${SHADER_FORMAT} STREQUAL "GLSL")
  add_definitions(-DUSE_GLSL_SHADERS)
elseif (${SHADER_FORMAT} STREQUAL "CG")
  add_definitions(-DUSE_CG_SHADERS)
elseif (${SHADER_FORMAT} STREQUAL "GXP")
  add_definitions(-DUSE_GXP_SHADERS)
else()
  message(FATAL_ERROR "SHADER_FORMAT option can only be one of: 'GLSL', 'CG', 'GXP'")
endif()

set(valid_dump_formats GLSL CG)
option(DUMP_COMPILED_SHADERS "When using GLSL or CG shader format, cache compiled shaders on disk" ON)
if (DUMP_COMPILED_SHADERS)
  if (SHADER_FORMAT IN_LIST valid_dump_formats)
    add_definitions(-DDUMP_COMPILED_SHADERS)
  else()
    message(WARNING "DUMP_COMPILED_SHADERS has no effect for the chosen shader format ${SHADER_FORMAT}")
  endif()
endif()

# Build configuration & Debug options
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

option(DEBUG_SOLOADER "Enable soloader debug logging" OFF)
option(DEBUG_OPENGL "Enable OpenGL / shader debug logging" OFF)

# Bug 16 (PORTING_PLAN.md) perf diagnostic: count glDrawArrays/glDrawElements/
# glBindTexture calls per frame to find the real GL-pipeline hot path (the
# equivalent of Zenonia 4's PutCompressImg call-count probe). Off by default:
# adds a branch + counter increment per draw/bind call.
option(INSTRUMENT_GL_CALLS "Log per-frame GL draw-call/texture-bind counts" OFF)
if (INSTRUMENT_GL_CALLS)
  add_definitions(-DINSTRUMENT_GL_CALLS)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_SOLOADER ON CACHE BOOL "Enable soloader debug logging" FORCE)
endif()

if (DEBUG_SOLOADER)
  add_definitions(-DDEBUG_SOLOADER)
endif()

if (DEBUG_OPENGL)
  add_definitions(-DDEBUG_OPENGL)
endif()

if (DEFINED FALSOJNI_DEBUGLEVEL)
  if (FALSOJNI_DEBUGLEVEL STREQUAL "ALL" OR FALSOJNI_DEBUGLEVEL STREQUAL "0")
    add_definitions(-DFALSOJNI_DEBUGLEVEL=0)
  elseif (FALSOJNI_DEBUGLEVEL STREQUAL "INFO" OR FALSOJNI_DEBUGLEVEL STREQUAL "1")
    add_definitions(-DFALSOJNI_DEBUGLEVEL=1)
  elseif (FALSOJNI_DEBUGLEVEL STREQUAL "WARN" OR FALSOJNI_DEBUGLEVEL STREQUAL "2")
    add_definitions(-DFALSOJNI_DEBUGLEVEL=2)
  elseif (FALSOJNI_DEBUGLEVEL STREQUAL "ERROR" OR FALSOJNI_DEBUGLEVEL STREQUAL "3")
    add_definitions(-DFALSOJNI_DEBUGLEVEL=3)
  elseif (FALSOJNI_DEBUGLEVEL STREQUAL "NO" OR FALSOJNI_DEBUGLEVEL STREQUAL "4")
    add_definitions(-DFALSOJNI_DEBUGLEVEL=4)
  else()
    add_definitions(-DFALSOJNI_DEBUGLEVEL=${FALSOJNI_DEBUGLEVEL})
  endif()
endif()

# makes sincos, sincosf, etc. visible
add_definitions(-D_GNU_SOURCE -D__POSIX_VISIBLE=999999)

# -mcpu=cortex-a9 -mfpu=neon: the vita.toolchain.cmake toolchain file does
# NOT set these itself (only CMAKE_SYSTEM_PROCESSOR "armv7-a"), so without
# them the compiler falls back to a generic ARMv7 target and won't emit NEON
# vector instructions for this port's own C/C++ code (the prebuilt vitasdk
# libs it links against, e.g. mathneon/vitaGL, are already built cortex-a9/
# neon, so this just brings our own object files in line with them).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -ffast-math -mfloat-abi=softfp -Wno-deprecated -mcpu=cortex-a9 -mfpu=neon")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-O2 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++20 -Wno-write-strings -Wno-psabi")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")

# NOTE on -flto: deliberately NOT enabled. Confirmed by an actual release
# build attempt that it breaks the link: dynlib.c declares C++ runtime
# symbols used by the .so (operator new, __cxa_throw_bad_alloc, etc.) as
# plain `extern void *` so their addresses can be resolved generically into
# the soloader's import table (same pattern as every other symbol in that
# file's dynlib_hooks[] table). Under LTO, GCC cross-checks those
# declarations against libstdc++'s real function prototypes for the same
# symbols pulled in elsewhere and fails with "redeclared as function"
# (lto1: fatal error: errors during merging of translation units). Fixing
# this would mean reworking dynlib.c's symbol table pattern for every
# conflicting libstdc++ symbol -- out of scope for a perf tweak.

add_executable(${CMAKE_PROJECT_NAME}
               source/main.c
               source/dynlib.c
               source/java.c
               source/audio.c
               source/font.c
               source/patch.c
               source/reimpl/bits/_ctype.c
               source/reimpl/egl.c
               source/reimpl/errno.c
               source/reimpl/io.c
               source/reimpl/log.c
               source/reimpl/mem.c
               source/reimpl/pthr.c
               source/reimpl/sys.c
               source/reimpl/time64.c
               source/reimpl/asset_manager.cpp
               source/utils/dialog.c
               source/utils/glutil.c
               source/utils/init.c
               source/utils/logger.c
               source/utils/settings.c
               source/utils/utils.c
               lib/falso_jni/FalsoJNI.c
               lib/falso_jni/FalsoJNI_ImplBridge.c
               lib/falso_jni/FalsoJNI_Logger.c
               lib/sha1/sha1.c
               lib/fios/fios.c
               lib/so_util/so_util.c)

add_subdirectory(lib/libc_bridge)
add_dependencies(${CMAKE_PROJECT_NAME} SceLibcBridge)

target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                           PUBLIC ${CMAKE_SOURCE_DIR}/lib
                           )

target_include_directories(${CMAKE_PROJECT_NAME}
                           PUBLIC ${CMAKE_SOURCE_DIR}/source
                           )

target_link_libraries(${CMAKE_PROJECT_NAME}
                      -Wl,--whole-archive pthread -Wl,--no-whole-archive
                      stdc++
                      vitaGL
                      vitashark
                      vorbisidec
                      ogg
                      SceShaccCgExt
                      taihen_stub
                      SceShaccCg_stub
                      z
                      m
                      ScePgf_stub
                      mathneon
                      kubridge_stub
                      ${CMAKE_BINARY_DIR}/lib/libc_bridge/stubs/libSceLibcBridge_stub.a
                      SceHid_stub
                      SceTouch_stub
                      SceMotion_stub
                      SceAppMgr_stub
                      SceAppUtil_stub
                      SceAudio_stub
                      SceAudioIn_stub
                      SceCommonDialog_stub
                      SceDisplay_stub
                      SceKernelDmacMgr_stub
                      SceFios2_stub
                      SceCtrl_stub
                      SceGxm_stub
                      ScePower_stub
                      SceTouch_stub
                      SceVshBridge_stub
                      )

vita_create_self(eboot.bin ${CMAKE_PROJECT_NAME} UNSAFE NOASLR)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk_param.sfo
  COMMAND "${VITASDK}/bin/vita-mksfoex" -s APP_VER=${VITA_VERSION} -s TITLE_ID=${VITA_TITLEID} -d ATTRIBUTE2=12 ${VITA_APP_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk_param.sfo
  DEPENDS eboot.bin-self
  COMMENT "Generating param.sfo for ${VITA_VPKNAME}.vpk"
  VERBATIM
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk
  COMMAND "${VITASDK}/bin/vita-pack-vpk"
    -s "${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk_param.sfo"
    -b "${CMAKE_CURRENT_BINARY_DIR}/eboot.bin"
    -a "${CMAKE_SOURCE_DIR}/extras/cpuinfo=cpuinfo"
    -a "${CMAKE_SOURCE_DIR}/extras/meminfo=meminfo"
    -a "${CMAKE_SOURCE_DIR}/extras/livearea/icon0.png=sce_sys/icon0.png"
    -a "${CMAKE_SOURCE_DIR}/extras/livearea/pic0.png=sce_sys/pic0.png"
    -a "${CMAKE_SOURCE_DIR}/extras/livearea/startup.png=sce_sys/livearea/contents/startup.png"
    -a "${CMAKE_SOURCE_DIR}/extras/livearea/bg0.png=sce_sys/livearea/contents/bg0.png"
    -a "${CMAKE_SOURCE_DIR}/extras/livearea/template.xml=sce_sys/livearea/contents/template.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk_param.sfo"
  DEPENDS eboot.bin-self
  COMMENT "Building vpk ${VITA_VPKNAME}.vpk"
  VERBATIM
)

add_custom_target(${VITA_VPKNAME}.vpk-vpk ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${VITA_VPKNAME}.vpk"
)

add_custom_target(send
                  COMMAND echo destroy | nc ${PSVITAIP} 1338
                  COMMAND echo destroy | nc ${PSVITAIP} 1338
                  COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
                  COMMAND echo launch ${VITA_TITLEID} | nc ${PSVITAIP} 1338
                  DEPENDS ${VITA_VPKNAME}.vpk-vpk
                  )

add_custom_target(send_kvdb
                  COMMAND echo destroy | nc ${PSVITAIP} 1338
                  COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
                  COMMAND echo launch ux0:/app/${VITA_TITLEID}/eboot.bin | nc ${PSVITAIP} 1338
                  DEPENDS ${VITA_VPKNAME}.vpk-vpk
                  )

add_custom_target(dump
                  COMMAND ${CMAKE_SOURCE_DIR}/extras/scripts/get_dump.sh ${PSVITAIP}
                  COMMAND python3 ${PARSECOREPATH} coredump ${CMAKE_PROJECT_NAME}
                  )

add_custom_target(reboot
                  COMMAND echo reboot | nc ${PSVITAIP} 1338
                  )
