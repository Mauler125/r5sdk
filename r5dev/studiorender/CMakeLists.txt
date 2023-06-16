cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "studiorender" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "studiorendercontext.cpp"
    "studiorendercontext.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/model_types.h"
    "${ENGINE_SOURCE_DIR}/public/studio.h"

    # TODO: These are from bsplib:
    "${ENGINE_SOURCE_DIR}/public/bspflags.h"
    "${ENGINE_SOURCE_DIR}/public/worldsize.h"
)

end_sources()
