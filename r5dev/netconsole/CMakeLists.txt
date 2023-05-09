cmake_minimum_required( VERSION 3.16 )

project( netconsole )
add_executable( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "netconsole.cpp"
    "netconsole.h"
    "plat_time.cpp"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME "netcon32" )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "libprotobuf"
    "libspdlog"
    "protocol_pb"
    "engine_ds"
    "Rpcrt4.lib"
    "ws2_32.lib"
)
