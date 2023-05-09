cmake_minimum_required( VERSION 3.16 )

project( pluginsystem )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "ipluginsystem.h"
    "modsystem.cpp"
    "modsystem.h"
    "pluginsystem.cpp"
    "pluginsystem.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
