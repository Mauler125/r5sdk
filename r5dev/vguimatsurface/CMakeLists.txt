cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vguimatsurface" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "MatSystemSurface.cpp"
    "MatSystemSurface.h"
)

end_sources()
