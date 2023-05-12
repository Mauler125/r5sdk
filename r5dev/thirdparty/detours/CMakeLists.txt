cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libdetours" "" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "src/creatwth.cpp"
    "src/detours.cpp"
    "src/disasm.cpp"
    "src/modules.cpp"
    "include/detours.h"
    "include/detver.h"
    "include/idetour.h"
    "include/syelog.h"
)

end_sources()
