cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "materialsystem" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "cmaterialglue.cpp"
    "cmaterialglue.h"
    "cmaterialsystem.cpp"
    "cmaterialsystem.h"
    "cshaderglue.cpp"
    "cshaderglue.h"
)

end_sources()
