cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vstdlib" "vpc" ${FOLDER_CONTEXT} )

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

target_include_directories( ${PROJECT_NAME} PRIVATE
    "${ENGINE_SOURCE_DIR}/tier0/"
    "${ENGINE_SOURCE_DIR}/tier1/"
)
