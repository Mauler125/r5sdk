cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "game" "vpc" ${FOLDER_CONTEXT} )

start_sources()

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
    "shared/usercmd.h"
    "shared/util_shared.cpp"
    "shared/util_shared.h"
    "shared/vscript_shared.cpp"
    "shared/vscript_shared.h"
)

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
    "server/detour_impl.h"
    "server/entitylist.cpp"
    "server/entitylist.h"
    "server/fairfight_impl.h"
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
)

add_sources( SOURCE_GROUP "Client"
    "client/c_baseentity.cpp"
    "client/c_baseentity.h"
    "client/c_baseplayer.h"
    "client/cliententitylist.h"
    "client/enginesprite.h"
    "client/hud.h"
    "client/movehelper_client.cpp"
    "client/movehelper_client.h"
    "client/spritemodel.cpp"
    "client/viewrender.cpp"
    "client/viewrender.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE
    "${ENGINE_SOURCE_DIR}/tier0/"
    "${ENGINE_SOURCE_DIR}/tier1/"
)
