cmake_minimum_required( VERSION 3.16 )
add_module( "shared_lib" "gamesdk" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "${ENGINE_SOURCE_DIR}/resource/r5dev.rc"
    "assert.h"
    "dllmain.cpp"
    "init.cpp"
    "init.h"
    "logdef.cpp"
    "logdef.h"
    "logger.cpp"
    "logger.h"
    "r5dev.h"
    "resource.h"
    "shared_pch.h"
    "stdafx.cpp"
    "stdafx.h"
    "termutil.cpp"
    "termutil.h"
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"
    "tier2"
    "vpklib"
    "vscript"
    "vstdlib"
    "vphysics"
    "vguimatsurface"
    "vpc"
    "vgui"

    "rtech_tools"
    "rtech_game"
    "rui"
    "stryder"

    "mathlib"

    "libdetours"
    "liblzham"
    "libimgui"
    "libcurl"
    "libprotobuf"
    "libspdlog"
    "navdebugutils"
    "libdetour"
    "protocol_pb"

    "networksystem"
    "pluginsystem"
    "materialsystem"
    "inputsystem"
    "filesystem"
    "datacache"
    "EbisuSDK"
    "codecs"

    "localize"

    "game"

    "engine"

    "appframework"

    "advapi32.lib"
    "bcrypt.lib"
    "crypt32.lib"
    "dbghelp.lib"
    "d3d11.lib"
    "wldap32.lib"
    "ws2_32.lib"
    "Rpcrt4.lib"
)

end_sources()

target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "GAMESDK"
)
target_link_options( ${PROJECT_NAME} PRIVATE
    "/STACK:8000000" # Match game executable stack reserve size
)
