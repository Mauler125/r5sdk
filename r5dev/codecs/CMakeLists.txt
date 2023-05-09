cmake_minimum_required( VERSION 3.16 )

project( codecs )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Bink"
    "bink/bink_impl.cpp"
    "bink/bink_impl.h"
)

add_sources( SOURCE_GROUP "Miles"
    "miles/miles_impl.cpp"
    "miles/miles_impl.h"
    "miles/miles_types.h" # TODO[ AMOS ]: move to public!
    "miles/radshal_wasapi.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" "${ENGINE_SOURCE_DIR}/tier1/" )
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM tier0 )
