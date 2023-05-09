cmake_minimum_required( VERSION 3.16 )

project( launcher )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "launcher.cpp"
    "launcher.h"
    "launcherdefs.h"
    "prx.cpp"
    "prx.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
