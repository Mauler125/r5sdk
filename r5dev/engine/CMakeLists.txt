cmake_minimum_required( VERSION 3.16 )

macro( add_engine_project PROJECT_NAME )
add_module( "lib" ${PROJECT_NAME} "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Engine"
    "clockdriftmgr.cpp"
    "clockdriftmgr.h"
    "cmd.cpp"
    "cmd.h"
    "common.cpp"
    "common.h"
)

add_sources( SOURCE_GROUP "Collision"
    "enginetrace.cpp"
    "enginetrace.h"
    "traceinit.h"
)

if( ${PROJECT_NAME} STREQUAL "engine" )
add_sources( SOURCE_GROUP "Debug"
    "debugoverlay.cpp"
    "debugoverlay.h"
)
endif()

if( ${PROJECT_NAME} STREQUAL "engine" )
add_sources( SOURCE_GROUP "Render"
    "gl_matsysiface.h"
    "gl_model_private.h"
    "gl_rmain.cpp"
    "gl_rmain.h"
    "gl_rsurf.cpp"
    "gl_rsurf.h"
    "gl_screen.cpp"
    "gl_screen.h"
    "matsys_interface.cpp"
    "matsys_interface.h"
)
endif()

add_sources( SOURCE_GROUP "Network"
    "datablock.h"
    "framesnapshot.h"
    "net.cpp"
    "net.h"
    "net_chan.cpp"
    "net_chan.h"
    "networkstringtable.cpp"
    "networkstringtable.h"
    "packed_entity.h"
    "staticpropmgr.cpp"
    "staticpropmgr.h"
)

add_sources( SOURCE_GROUP "Model"
    "cmodel_bsp.cpp"
    "cmodel_bsp.h"
    "modelinfo.cpp"
    "modelinfo.h"
    "modelloader.cpp"
    "modelloader.h"
)

add_sources( SOURCE_GROUP "Host"
    "host.cpp"
    "host.h"
    "host_cmd.cpp"
    "host_cmd.h"
    "host_state.cpp"
    "host_state.h"
)

add_sources( SOURCE_GROUP "System"
    "sys_dll.cpp"
    "sys_dll.h"
    "sys_dll2.cpp"
    "sys_dll2.h"
    "sys_engine.cpp"
    "sys_engine.h"
    "sys_utils.cpp"
    "sys_utils.h"
    "sdk_dll.cpp"
    "sdk_dll.h"
)

if( ${PROJECT_NAME} STREQUAL "engine" )
add_sources( SOURCE_GROUP "System"
    "sys_getmodes.cpp"
    "sys_getmodes.h"
    "sys_mainwind.cpp"
    "sys_mainwind.h"
)
endif()

add_sources( SOURCE_GROUP "Server"
    "server/persistence.cpp"
    "server/persistence.h"
    "server/server.cpp"
    "server/server.h"
    "server/sv_main.cpp"
    "server/sv_main.h"
    "server/sv_rcon.cpp"
    "server/sv_rcon.h"
    "server/vengineserver_impl.cpp"
    "server/vengineserver_impl.h"
)

add_sources( SOURCE_GROUP "Client"
    "client/cdll_engine_int.cpp"
    "client/cdll_engine_int.h"
    "client/cl_ents_parse.cpp"
    "client/cl_ents_parse.h"
    "client/cl_main.h"
    "client/client.cpp"
    "client/client.h"
)

add_sources( SOURCE_GROUP "Shared"
    "shared/base_rcon.cpp"
    "shared/base_rcon.h"
    "shared/shared_rcon.cpp"
    "shared/shared_rcon.h"
)

if( ${PROJECT_NAME} STREQUAL "engine" )
add_sources( SOURCE_GROUP "Client" # Client only.
    "client/clientstate.cpp"
    "client/clientstate.h"
    "client/cl_rcon.cpp"
    "client/cl_rcon.h"
    "client/vengineclient_impl.cpp"
    "client/vengineclient_impl.h"
)
endif()

if( ${PROJECT_NAME} STREQUAL "engine" )
add_sources( SOURCE_GROUP "GameUI"
    "${ENGINE_SOURCE_DIR}/gameui/IBrowser.cpp"
    "${ENGINE_SOURCE_DIR}/gameui/IBrowser.h"
    "${ENGINE_SOURCE_DIR}/gameui/IConsole.cpp"
    "${ENGINE_SOURCE_DIR}/gameui/IConsole.h"
)
endif()

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/console.cpp"
    "${ENGINE_SOURCE_DIR}/windows/console.h"
    "${ENGINE_SOURCE_DIR}/windows/id3dx.cpp"
    "${ENGINE_SOURCE_DIR}/windows/id3dx.h"
    "${ENGINE_SOURCE_DIR}/windows/input.cpp"
    "${ENGINE_SOURCE_DIR}/windows/input.h"
    "${ENGINE_SOURCE_DIR}/windows/resource.cpp"
    "${ENGINE_SOURCE_DIR}/windows/resource.h"
    "${ENGINE_SOURCE_DIR}/windows/system.cpp"
    "${ENGINE_SOURCE_DIR}/windows/system.h"
)

add_sources( SOURCE_GROUP "Common"
    "${ENGINE_SOURCE_DIR}/common/callback.cpp"
    "${ENGINE_SOURCE_DIR}/common/callback.h"
    "${ENGINE_SOURCE_DIR}/common/completion.cpp"
    "${ENGINE_SOURCE_DIR}/common/completion.h"
    "${ENGINE_SOURCE_DIR}/common/engine_launcher_api.h"
    "${ENGINE_SOURCE_DIR}/common/experimental.h"
    "${ENGINE_SOURCE_DIR}/common/global.cpp"
    "${ENGINE_SOURCE_DIR}/common/global.h"
    "${ENGINE_SOURCE_DIR}/common/igameserverdata.h"
    "${ENGINE_SOURCE_DIR}/common/netmessages.cpp"
    "${ENGINE_SOURCE_DIR}/common/netmessages.h"
    "${ENGINE_SOURCE_DIR}/common/opcodes.cpp"
    "${ENGINE_SOURCE_DIR}/common/opcodes.h"
    "${ENGINE_SOURCE_DIR}/common/protocol.h"
    "${ENGINE_SOURCE_DIR}/common/pseudodefs.h"
    "${ENGINE_SOURCE_DIR}/common/qlimits.h"
    "${ENGINE_SOURCE_DIR}/common/sdkdefs.h"
    "${ENGINE_SOURCE_DIR}/common/x86defs.h"
)

end_sources()

if( ${PROJECT_NAME} STREQUAL "engine_ds" )
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "DEDICATED"
)
endif()
endmacro()

add_engine_project( "engine" )
add_engine_project( "engine_ds" )
