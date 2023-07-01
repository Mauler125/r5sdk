cmake_minimum_required( VERSION 3.16 )

# Separate lib as we aren't implementing the memalloc system!
# This library mainly interfaces the program to the game's
# internal memalloc system. Only link if its getting used or
# is using the game module!!!
add_module( "lib" "memoverride" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

add_sources( SOURCE_GROUP "Runtime"
    "memstd.cpp"
    "memstd.h"
)

end_sources()

add_module( "lib" "tier0" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Debug"
    "dbg.cpp"
    "fasttimer.cpp"
)

add_sources( SOURCE_GROUP "CPU"
    "cpu.cpp"
    "cpu.h"
    "cputopology.cpp"
    "cputopology.h"
)

add_sources( SOURCE_GROUP "Runtime"
    "commandline.cpp"
    "commandline.h"
    "crashhandler.cpp"
    "frametask.cpp"
    "jobthread.cpp"
    "memaddr.cpp"
    "module.cpp"
    "module_statics.cpp"
    "platform.cpp"
    "sigcache.cpp"
    "threadtools.cpp"
    "tslist.cpp"
    "vtable.cpp"
    "tier0_iface.cpp"
    "utility.cpp"
    "binstream.cpp"
)

add_sources( SOURCE_GROUP "Windows"
    "${ENGINE_SOURCE_DIR}/windows/tebpeb64.h"
)

file( GLOB TIER0_PUBLIC_HEADERS
    "${ENGINE_SOURCE_DIR}/public/tier0/*"
)
add_sources( SOURCE_GROUP "Public"
    "${TIER0_PUBLIC_HEADERS}"
    "${ENGINE_SOURCE_DIR}/public/icommandline.h"
    "${ENGINE_SOURCE_DIR}/public/iframetask.h"
)

end_sources()
