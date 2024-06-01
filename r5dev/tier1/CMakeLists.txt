cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "tier1" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

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
    "utlsymbol.cpp"
    "characterset.cpp"
    "mempool.cpp"
    "memstack.cpp"
    "exprevaluator.cpp"
    "keyvalues.cpp"
)

add_sources( SOURCE_GROUP "Private"
    "cmd.cpp"
    "commandbuffer.cpp"
    "convar.cpp"
    "cvar.cpp"
    "interface.cpp"
    "keyvalues_iface.cpp"
    "kvleaktrace.h"
    "kverrorstack.h"
    "kvtokenreader.h"
)

file( GLOB TIER1_PUBLIC_HEADERS
    "${ENGINE_SOURCE_DIR}/public/tier1/*"
)
add_sources( SOURCE_GROUP "Public"
    "${TIER1_PUBLIC_HEADERS}"
    "${ENGINE_SOURCE_DIR}/public/iconcommand.h"
    "${ENGINE_SOURCE_DIR}/public/iconvar.h"
    "${ENGINE_SOURCE_DIR}/public/icvar.h"
)

end_sources()

target_compile_definitions( ${PROJECT_NAME} PRIVATE "MEM_DEBUG_CLASSNAME" )
target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" )
