cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "EbisuSDK" "tier0" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "EbisuSDK.cpp"
    "EbisuSDK.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/ebisusdk/EbisuTypes.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" "${ENGINE_SOURCE_DIR}/tier1/" )
