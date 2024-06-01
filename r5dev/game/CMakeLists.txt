cmake_minimum_required( VERSION 3.16 )

macro( add_game_project PROJECT_NAME )
add_module( "lib" ${PROJECT_NAME} "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

if( ${PROJECT_NAME} STREQUAL "game_shared_static" )

add_sources( SOURCE_GROUP "AI"
    "shared/ai_utility_shared.cpp"
    "shared/ai_utility_shared.h"
)

add_sources( SOURCE_GROUP "Entity"
    "shared/ehandle.h"
    "shared/entitylist_base.cpp"
    "shared/entitylist_base.h"
)

add_sources( SOURCE_GROUP "Prediction"
    "shared/predictioncopy.h"
)

add_sources( SOURCE_GROUP "Animation"
    "shared/animation.cpp"
    "shared/animation.h"
)

add_sources( SOURCE_GROUP "Collision"
    "shared/collisionproperty.cpp"
    "shared/collisionproperty.h"
)

add_sources( SOURCE_GROUP "Network"
    "shared/playernet_vars.h"
    "shared/usercmd.cpp"
    "shared/usercmd.h"
    "shared/usermessages.h"
)

add_sources( SOURCE_GROUP "Utility"
    "shared/imovehelper.h"
    "shared/util_shared.cpp"
    "shared/util_shared.h"
)

add_sources( SOURCE_GROUP "Shared"
    "shared/shared_classnames.h"
    "shared/shareddefs.h"
    "shared/takedamageinfo.h"
)

add_sources( SOURCE_GROUP "Script"
    "shared/vscript_shared.cpp"
    "shared/vscript_shared.h"
)

add_sources( SOURCE_GROUP "Weapon"
    "shared/r1/weapon_bolt.cpp"
    "shared/r1/weapon_bolt.h"
)

endif()

if( ${PROJECT_NAME} STREQUAL "server_static" )

add_sources( SOURCE_GROUP "AI"
    "server/ai_network.cpp"
    "server/ai_network.h"
    "server/ai_networkmanager.cpp"
    "server/ai_networkmanager.h"
    "server/ai_node.h"
    "server/ai_utility.cpp"
    "server/ai_utility.h"
    "server/detour_impl.h"
)

add_sources( SOURCE_GROUP "Entity"
    "server/baseanimating.cpp"
    "server/baseanimating.h"
    "server/baseanimatingoverlay.h"
    "server/basecombatcharacter.h"
    "server/baseentity.cpp"
    "server/baseentity.h"
    "server/entitylist.cpp"
    "server/entitylist.h"
    "server/entityoutput.cpp"
    "server/entityoutput.h"
)

add_sources( SOURCE_GROUP "Network"
    "server/networkproperty.cpp"
    "server/networkproperty.h"
)

add_sources( SOURCE_GROUP "Player"
    "server/player.cpp"
    "server/player.h"
    "server/player_command.cpp"
    "server/player_command.h"
    "server/playerlocaldata.h"
)

add_sources( SOURCE_GROUP "Script"
    "server/vscript_server.cpp"
    "server/vscript_server.h"
)

add_sources( SOURCE_GROUP "Physics"
    "server/physics_main.cpp"
    "server/physics_main.h"
)

add_sources( SOURCE_GROUP "Utility"
    "server/cbase.cpp"
    "server/cbase.h"
    "server/gameinterface.cpp"
    "server/gameinterface.h"
    "server/movehelper_server.cpp"
    "server/movehelper_server.h"
    "server/util_server.cpp"
    "server/util_server.h"
    "server/variant_t.cpp"
    "server/variant_t.h"
)

add_sources( SOURCE_GROUP "LiveAPI"
    "server/liveapi/liveapi.cpp"
    "server/liveapi/liveapi.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/iserverentity.h"
    "${ENGINE_SOURCE_DIR}/public/iservernetworkable.h"
    "${ENGINE_SOURCE_DIR}/public/iserverunknown.h"
)

endif()

if( ${PROJECT_NAME} STREQUAL "client_static" )

add_sources( SOURCE_GROUP "Entity"
    "client/c_baseentity.cpp"
    "client/c_baseentity.h"
    "client/cliententitylist.h"
)

add_sources( SOURCE_GROUP "Player"
    "client/c_baseplayer.h"
)

add_sources( SOURCE_GROUP "Input"
    "client/input.cpp"
    "client/input.h"
)

add_sources( SOURCE_GROUP "Rendering"
    "client/enginesprite.h"
    "client/hud.h"
    "client/spritemodel.cpp"
    "client/viewrender.cpp"
    "client/viewrender.h"
)

add_sources( SOURCE_GROUP "Utility"
    "client/movehelper_client.cpp"
    "client/movehelper_client.h"
    "client/util_client.cpp"
    "client/util_client.h"
)

add_sources( SOURCE_GROUP "Script"
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
    "${THIRDPARTY_SOURCE_DIR}/mbedtls/include"
)

endmacro()

add_game_project( "server_static" )
add_game_project( "client_static" )
add_game_project( "game_shared_static" )
