cmake_minimum_required( VERSION 3.16 )
add_module( "shared_lib" "loader" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "loader.cpp"
    "loader.h"
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    "advapi32.lib"
    "bcrypt.lib"
    "crypt32.lib"
    "dbghelp.lib"
    "wldap32.lib"
    "ws2_32.lib"
    "Rpcrt4.lib"

    "vpc"
    "tier0"
    "tier1"

    "libdetours"
)

end_sources()

target_link_options( ${PROJECT_NAME} PRIVATE
    "/STACK:8000000" # Match game executable stack reserve size
)
