cmake_minimum_required( VERSION 3.16 )
add_module( "shared_lib" "PluginSDK" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "dllmain.cpp"
    "ifactory.h"
    "pluginsdk.cpp"
    "pluginsdk.h"
)

end_sources( "${BUILD_OUTPUT_DIR}/bin/x64_retail/plugins/" )

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "memoverride"

    "libdetours"
    "libprotobuf"
    "liblzham"
    "libspdlog"

    "SigCache_Pb"
    "SV_RCon_Pb"
    "CL_RCon_Pb"
    "Rpcrt4.lib"
)

target_link_options( ${PROJECT_NAME} PRIVATE
    "/STACK:8000000" # Match game executable stack reserve size
)
