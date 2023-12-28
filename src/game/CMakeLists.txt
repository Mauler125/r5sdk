cmake_minimum_required( VERSION 3.16 )

macro( add_game_project PROJECT_NAME )
add_module( "lib" ${PROJECT_NAME} "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

if( ${PROJECT_NAME} STREQUAL "game_shared_static" )

add_sources( SOURCE_GROUP "Shared"
    "shared/ai_utility_shared.cpp"
    "shared/ai_utility_shared.h"
    "shared/animation.cpp"
    "shared/animation.h"
    "shared/collisionproperty.cpp"
    "shared/collisionproperty.h"
    "shared/ehandle.h"
    "shared/entitylist_base.cpp"
    "shared/entitylist_base.h"
    "shared/imovehelper.h"
    "shared/playernet_vars.h"
    "shared/predictioncopy.h"
    "shared/shared_classnames.h"
    "shared/shareddefs.h"
    "shared/takedamageinfo.h"
    "shared/usercmd.cpp"
    "shared/usercmd.h"
    "shared/usermessages.h"
    "shared/util_shared.cpp"
    "shared/util_shared.h"
    "shared/vscript_shared.cpp"
    "shared/vscript_shared.h"
)

add_sources( SOURCE_GROUP "Shared/Weapon"
    "shared/r1/weapon_bolt.cpp"
    "shared/r1/weapon_bolt.h"
)

endif()

if( ${PROJECT_NAME} STREQUAL "server_static" )

add_sources( SOURCE_GROUP "Server"
    "server/ai_network.cpp"
    "server/ai_network.h"
    "server/ai_networkmanager.cpp"
    "server/ai_networkmanager.h"
    "server/ai_node.h"
    "server/ai_utility.cpp"
    "server/ai_utility.h"
    "server/baseanimating.cpp"
    "server/baseanimating.h"
    "server/baseanimatingoverlay.h"
    "server/basecombatcharacter.h"
    "server/baseentity.cpp"
    "server/baseentity.h"
    "server/cbase.cpp"
    "server/cbase.h"
    "server/detour_impl.h"
    "server/entitylist.cpp"
    "server/entitylist.h"
    "server/entityoutput.cpp"
    "server/entityoutput.h"
    "server/gameinterface.cpp"
    "server/gameinterface.h"
    "server/movehelper_server.cpp"
    "server/movehelper_server.h"
    "server/networkproperty.cpp"
    "server/networkproperty.h"
    "server/physics_main.cpp"
    "server/physics_main.h"
    "server/player.cpp"
    "server/player.h"
    "server/playerlocaldata.h"
    "server/util_server.cpp"
    "server/util_server.h"
    "server/variant_t.cpp"
    "server/variant_t.h"
    "server/vscript_server.cpp"
    "server/vscript_server.h"
    "server/logger.cpp"
    "server/logger.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/iserverentity.h"
    "${ENGINE_SOURCE_DIR}/public/iservernetworkable.h"
    "${ENGINE_SOURCE_DIR}/public/iserverunknown.h"
)

endif()

if( ${PROJECT_NAME} STREQUAL "client_static" )

add_sources( SOURCE_GROUP "Client"
    "client/c_baseentity.cpp"
    "client/c_baseentity.h"
    "client/c_baseplayer.h"
    "client/cliententitylist.h"
    "client/enginesprite.h"
    "client/hud.h"
    "client/input.cpp"
    "client/input.h"
    "client/movehelper_client.cpp"
    "client/movehelper_client.h"
    "client/spritemodel.cpp"
    "client/util_client.cpp"
    "client/util_client.h"
    "client/viewrender.cpp"
    "client/viewrender.h"
    "client/vscript_client.cpp"
    "client/vscript_client.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/icliententity.h"
    "${ENGINE_SOURCE_DIR}/public/icliententitylist.h"
    "${ENGINE_SOURCE_DIR}/public/iclientnetworkable.h"
    "${ENGINE_SOURCE_DIR}/public/iclientrenderable.h"
    "${ENGINE_SOURCE_DIR}/public/iclientthinkable.h"
    "${ENGINE_SOURCE_DIR}/public/iclientunknown.h"

    "${ENGINE_SOURCE_DIR}/public/game/client/iinput.h"
)

endif()

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/basehandle.h"
    "${ENGINE_SOURCE_DIR}/public/edict.h"
    "${ENGINE_SOURCE_DIR}/public/eiface.h"
    "${ENGINE_SOURCE_DIR}/public/globalvars_base.h"
    "${ENGINE_SOURCE_DIR}/public/ihandleentity.h"

    "${ENGINE_SOURCE_DIR}/public/game/shared/weapon_types.h"
    "${ENGINE_SOURCE_DIR}/public/game/shared/in_buttons.h"
)

end_sources()

if( ${PROJECT_NAME} STREQUAL "server_static" )

target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "SERVER_DLL"
)
elseif( ${PROJECT_NAME} STREQUAL "client_static" )
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "CLIENT_DLL"
)

endif()

target_include_directories( ${PROJECT_NAME} PRIVATE
    "${ENGINE_SOURCE_DIR}/tier0/"
    "${ENGINE_SOURCE_DIR}/tier1/"
   "${ENGINE_SOURCE_DIR}/tiny-aes"
)

endmacro()

add_game_project( "server_static" )
add_game_project( "client_static" )
add_game_project( "game_shared_static" )
