cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "localize" "vpc" ${FOLDER_CONTEXT} TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "localize.cpp"
    "localize.h"
)

end_sources()
