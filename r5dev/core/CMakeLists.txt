cmake_minimum_required( VERSION 3.16 )
macro( add_sdk_project PROJECT_NAME )
add_module( "shared_lib" ${PROJECT_NAME} "vpc" ${FOLDER_CONTEXT} )

start_sources()

if( NOT ${PROJECT_NAME} STREQUAL "dedicated" )
file( GLOB PNG_SOURCES
    "${ENGINE_SOURCE_DIR}/resource/png/*.png"
)
add_sources( SOURCE_GROUP "Resource"
    "${ENGINE_SOURCE_DIR}/resource/r5dev.rc"
    "${PNG_SOURCES}"
)
endif()

add_sources( SOURCE_GROUP "Core"
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
    "advapi32.lib"
    "bcrypt.lib"
    "crypt32.lib"
    "dbghelp.lib"
    "wldap32.lib"
    "ws2_32.lib"
    "Rpcrt4.lib"

    "vpc"
    "tier0"
    "tier1"
    "tier2"
    "launcher"
    "appframework"

    "vstdlib"
    "vpklib"
    "mathlib"
    "protocol_pb"
    "vphysics"

    "rtech_tools"
    "rtech_game"
    "stryder"

    "libdetours"
    "liblzham"
    "libcurl"
    "libprotobuf"
    "libspdlog"
    "libdetour"
    "navdebugutils"

    "networksystem"
    "pluginsystem"
    "filesystem"
    "datacache"
    "EbisuSDK"

    "localize"

    "vscript"
    "game"
)

if( NOT ${PROJECT_NAME} STREQUAL "dedicated" )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "libimgui"
    "codecs"

    "inputsystem"
    "materialsystem"

    "vguimatsurface"
    "vgui"
    "rui"

    "engine"
    "d3d11.lib"
)
else()
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "engine_ds"
)
endif()

if( ${PROJECT_NAME} STREQUAL "gamesdk" )
end_sources()
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "GAMESDK"
)
elseif( ${PROJECT_NAME} STREQUAL "dedicated" )
end_sources()
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "DEDICATED"
)
elseif( ${PROJECT_NAME} STREQUAL "client" )
end_sources( "game/bin/x64_retail/" )
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "CLIENT_DLL"
)
endif()

target_link_options( ${PROJECT_NAME} PRIVATE
    "/STACK:8000000" # Match game executable stack reserve size
)

add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -P ${ENGINE_SOURCE_DIR}/cmake/PostBuild.cmake
)

endmacro()

add_sdk_project( "gamesdk" )
add_sdk_project( "dedicated" )
add_sdk_project( "client" )
