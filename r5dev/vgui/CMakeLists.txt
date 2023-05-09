cmake_minimum_required( VERSION 3.16 )

project( vgui )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "vgui.h"
    "vgui_baseui_interface.cpp"
    "vgui_baseui_interface.h"
    "vgui_debugpanel.cpp"
    "vgui_debugpanel.h"
    "vgui_fpspanel.cpp"
    "vgui_fpspanel.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
