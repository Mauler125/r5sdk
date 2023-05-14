cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "tier1" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Network"
    "NetAdr.cpp"
    "NetKey.cpp"
)

add_sources( SOURCE_GROUP "Utility"
    "bitbuf.cpp"
    "generichash.cpp"
    "lzss.cpp"
    "splitstring.cpp"
    "stringpool.cpp"
    "strtools.cpp"
    "utlbuffer.cpp"
    "utlstring.cpp"
    "characterset.cpp"
)

add_sources( SOURCE_GROUP "Private"
    "cmd.cpp"
    "cvar.cpp"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/tier1/bitbuf.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/byteswap.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/characterset.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/cmd.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/cvar.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/generichash.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/lzss.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/mempool.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/memstack.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/NetAdr.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/NetKey.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/stringpool.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/strtools.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlblockmemory.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlbuffer.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utldict.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlfixedmemory.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utllinkedlist.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlmap.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlmemory.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlrbtree.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlstring.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlsymbol.h"
    "${ENGINE_SOURCE_DIR}/public/tier1/utlvector.h"
)

end_sources()

target_compile_definitions( ${PROJECT_NAME} PRIVATE "MEM_DEBUG_CLASSNAME" )
target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" )
