cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "sdklauncher" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "sdklauncher.cpp"
    "sdklauncher.h"
    "sdklauncher_const.h"
    "sdklauncher_utils.cpp"
    "sdklauncher_utils.h"
)

add_sources( SOURCE_GROUP "GUI"
    "advanced_surface.cpp"
    "advanced_surface.h"
    "base_surface.cpp"
    "base_surface.h"
    "download_surface.cpp"
    "download_surface.h"
)

add_sources( SOURCE_GROUP "Resource"
    "sdklauncher_res.h"
    "${ENGINE_SOURCE_DIR}/resource/sdklauncher.rc"
)

end_sources()

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME
    "launcher"
)
set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "launcher.exe"
    VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)../../../${BUILD_OUTPUT_DIR}/"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "SDKLAUNCHER"
)
target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "sdklauncher_pch.h"
)
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"
    "tier2"
    "libdetours"
    "libcppkore"
    "libspdlog"
    "libcurl"

    "liblzma"
    "libzlib"
    "libbzip2"
    "libzip"

    "crypt32.lib"
    "ws2_32.lib"
    "wldap32.lib"
    "Rpcrt4.lib"
)

add_module( "exe" "sdkupdater" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "sdkupdater.cpp"
    "sdkupdater.h"
)

end_sources( "${BUILD_OUTPUT_DIR}/bin/" )

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME
    "updater"
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"
    "crypt32.lib"
    "ws2_32.lib"
    "wldap32.lib"
    "Rpcrt4.lib"
)