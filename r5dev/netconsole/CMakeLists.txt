cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "netconsole" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "netconsole.cpp"
    "netconsole.h"
    "plat_time.cpp"
    "${ENGINE_SOURCE_DIR}/core/logdef.cpp"
    "${ENGINE_SOURCE_DIR}/core/logdef.h"
    "${ENGINE_SOURCE_DIR}/core/logger.cpp"
    "${ENGINE_SOURCE_DIR}/core/logger.h"
    "${ENGINE_SOURCE_DIR}/core/termutil.cpp"
    "${ENGINE_SOURCE_DIR}/core/termutil.h"
)

add_sources( SOURCE_GROUP "Engine"
    "${ENGINE_SOURCE_DIR}/engine/net.cpp"
    "${ENGINE_SOURCE_DIR}/engine/net.h"
    "${ENGINE_SOURCE_DIR}/engine/shared/base_rcon.cpp"
    "${ENGINE_SOURCE_DIR}/engine/shared/base_rcon.h"
    "${ENGINE_SOURCE_DIR}/engine/shared/shared_rcon.cpp"
    "${ENGINE_SOURCE_DIR}/engine/shared/shared_rcon.h"
)

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/console.cpp"
    "${ENGINE_SOURCE_DIR}/windows/console.h"
)

end_sources( "game/bin/" )

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME
    "netcon32"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "NETCONSOLE"
)
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "tier1"
    "tier2"
    "libprotobuf"
    "libspdlog"
    "protocol_pb"
    "Rpcrt4.lib"
    "ws2_32.lib"
)
