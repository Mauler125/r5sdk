cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "sdklauncher" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Foundation"
    "${ENGINE_SOURCE_DIR}/tier0/plat_time.cpp"
    "${ENGINE_SOURCE_DIR}/public/tier0/platform.h"
)

add_sources( SOURCE_GROUP "Core"
    "${ENGINE_SOURCE_DIR}/core/logger.cpp"
    "${ENGINE_SOURCE_DIR}/core/logger.h"
    "${ENGINE_SOURCE_DIR}/core/logdef.cpp"
    "${ENGINE_SOURCE_DIR}/core/logdef.h"
    "${ENGINE_SOURCE_DIR}/core/termutil.cpp"
    "${ENGINE_SOURCE_DIR}/core/termutil.h"
)

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/console.cpp"
    "${ENGINE_SOURCE_DIR}/windows/console.h"
)

add_sources( SOURCE_GROUP "App"
    "sdklauncher.cpp"
    "sdklauncher.h"
    "sdklauncher_const.h"
)

add_sources( SOURCE_GROUP "GUI"
    "basepanel.cpp"
    "basepanel.h"
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
    "_TOOLS"
    "SDKLAUNCHER"
)
target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "sdklauncher_pch.h"
)
target_link_options( ${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS" )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"

    "filesystem_std"
    "vstdlib"

    "libdetours"
    "libcppkore"
    "libspdlog"
    "Rpcrt4.lib"
)
