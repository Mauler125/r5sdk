cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "launcher" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "launcher.cpp"
    "launcher.h"
    "prx.cpp"
    "prx.h"
)

end_sources()
