cmake_minimum_required( VERSION 3.16 )

project( tier0 )
add_library( ${PROJECT_NAME} )

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
    "memstd.h"
    "module.cpp"
    "platform.cpp"
    "sigcache.cpp"
    "threadtools.cpp"
    "tslist.cpp"
    "vtable.cpp"
    "tier0_pch.h"
    "tier0_iface.cpp"
    "utility.cpp"
    "binstream.cpp"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/tier0/annotations.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/basetypes.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/commonmacros.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/crashhandler.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/dbg.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/dbgflag.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/fasttimer.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/frametask.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/jobthread.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/memaddr.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/memalloc.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/memdbgoff.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/memdbgon.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/module.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/platform.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/platform_internal.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/sigcache.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/threadtools.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/tslist.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/valve_off.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/valve_on.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/vtable.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/wchartypes.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/tier0_iface.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/utility.h"
    "${ENGINE_SOURCE_DIR}/public/tier0/binstream.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} PRIVATE tier0_pch.h )
