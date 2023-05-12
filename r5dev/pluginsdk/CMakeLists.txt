cmake_minimum_required( VERSION 3.16 )
add_module( "shared_lib" "PluginSDK" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "dllmain.cpp"
    "ifactory.h"
    "pluginsdk.cpp"
    "pluginsdk.h"
)

end_sources()

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "libdetours"
    "libprotobuf"
    "liblzham"
    "protocol_pb"
    "Rpcrt4.lib"
)