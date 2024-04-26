cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "tier2" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Utility"
    "cryptutils.cpp"
    "curlutils.cpp"
    "fileutils.cpp"
    "meshutils.cpp"
    "renderutils.cpp"
    "socketcreator.cpp"
    "websocket.cpp"
)

file( GLOB TIER2_PUBLIC_HEADERS
    "${ENGINE_SOURCE_DIR}/public/tier2/*"
)
add_sources( SOURCE_GROUP "Public"
    "${TIER2_PUBLIC_HEADERS}"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE 
    "${ENGINE_SOURCE_DIR}/tier0/"
    "${ENGINE_SOURCE_DIR}/tier1/"
)

target_include_directories( ${PROJECT_NAME} PRIVATE 
    "${THIRDPARTY_SOURCE_DIR}/mbedtls/include"
    "${THIRDPARTY_SOURCE_DIR}/dirtysdk/include/"
    "${THIRDPARTY_SOURCE_DIR}/ea/"
)
