cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "revpk" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "revpk.cpp"
    "${ENGINE_SOURCE_DIR}/core/logdef.cpp"
    "${ENGINE_SOURCE_DIR}/core/logdef.h"
    "${ENGINE_SOURCE_DIR}/core/logger.cpp"
    "${ENGINE_SOURCE_DIR}/core/logger.h"
    "${ENGINE_SOURCE_DIR}/core/termutil.cpp"
    "${ENGINE_SOURCE_DIR}/core/termutil.h"
    "${ENGINE_SOURCE_DIR}/tier0/plat_time.cpp"
)

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/console.cpp"
    "${ENGINE_SOURCE_DIR}/windows/console.h"
)

end_sources( "${BUILD_OUTPUT_DIR}/bin/" )

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND "revpk.exe"
    VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)../../../${BUILD_OUTPUT_DIR}/bin/"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "_TOOLS"
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "vpc"
    "tier0"
    "tier1"
    "filesystem_std"
    "vstdlib"
    "mathlib"
    "vpklib"

    "libspdlog"
    "liblzham"
    "Rpcrt4.lib"
)
