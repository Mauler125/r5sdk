cmake_minimum_required( VERSION 3.16 )

project( PluginSDK )
add_library( ${PROJECT_NAME} SHARED )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "dllmain.cpp"
    "ifactory.h"
    "pluginsdk.cpp"
    "pluginsdk.h"
)

end_sources()

target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "libdetours"
    "libprotobuf"
    "liblzham"
    "protocol_pb"
    "Rpcrt4.lib"
)