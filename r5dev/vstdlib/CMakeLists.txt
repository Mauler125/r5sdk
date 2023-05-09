cmake_minimum_required( VERSION 3.16 )

project( vstdlib )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "autocompletefilelist.cpp"
    "autocompletefilelist.h"
    "concommandhash.h"
    "keyvaluessystem.cpp"
    "keyvaluessystem.h"
    "random.cpp"
    "random.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
target_include_directories( ${PROJECT_NAME} PRIVATE 
    "${ENGINE_SOURCE_DIR}/tier0/"
    "${ENGINE_SOURCE_DIR}/tier1/"
)
