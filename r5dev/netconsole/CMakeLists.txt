cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "netconsole" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "netconsole.cpp"
    "netconsole.h"
    "plat_time.cpp"
)

end_sources()

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
