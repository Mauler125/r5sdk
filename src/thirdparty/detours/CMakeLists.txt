cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libdetours" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "src/creatwth.cpp"
    "src/detours.cpp"
    "src/disasm.cpp"
    "src/idetour.cpp"
    "src/modules.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "include/detours.h"
    "include/detver.h"
    "include/idetour.h"
    "include/syelog.h"
)

end_sources()
