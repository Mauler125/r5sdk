cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vgui" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "vgui.h"
    "vgui_baseui_interface.cpp"
    "vgui_baseui_interface.h"
)

add_sources( SOURCE_GROUP "Debug"
    "vgui_debugpanel.cpp"
    "vgui_debugpanel.h"
    "vgui_fpspanel.cpp"
    "vgui_fpspanel.h"
)

end_sources()
