cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "sdklauncher" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "sdklauncher.cpp"
    "sdklauncher.h"
    "sdklauncher_const.h"
)

add_sources( SOURCE_GROUP "GUI"
    "basepanel.cpp"
    "basepanel.h"
)

add_sources( SOURCE_GROUP "Resource"
    "sdklauncher_res.h"
    "${ENGINE_SOURCE_DIR}/resource/sdklauncher.rc"
)

end_sources()

set_target_properties( ${PROJECT_NAME} PROPERTIES OUTPUT_NAME "launcher" )

target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "SDKLAUNCHER"
)
target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "launcher_pch.h"
)
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "tier0"
    "libdetours"
    "libcppkore"
    "libspdlog"
    "Rpcrt4.lib"
)
