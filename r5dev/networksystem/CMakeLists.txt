cmake_minimum_required( VERSION 3.16 )

project( networksystem )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "bansystem.cpp"
    "bansystem.h"
    "listmanager.cpp"
    "listmanager.h"
    "pylon.cpp"
    "pylon.h"
    "serverlisting.h"
    "sm_protocol.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
