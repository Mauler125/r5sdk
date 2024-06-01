cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "LiveAPI_Pb" "vpc" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "events.pb.cc"
    "events.pb.h"
)

end_sources()
thirdparty_suppress_warnings()

add_module( "lib" "SigCache_Pb" "vpc" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "sig_map.pb.cc"
    "sig_map.pb.h"
)

end_sources()
thirdparty_suppress_warnings()

add_module( "lib" "NetCon_Pb" "vpc" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "netcon.pb.cc"
    "netcon.pb.h"
)

end_sources()
thirdparty_suppress_warnings()
