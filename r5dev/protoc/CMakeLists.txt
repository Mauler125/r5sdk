cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "SigCache_Pb" "vpc" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "sig_map.pb.cc"
    "sig_map.pb.h"
)

end_sources()
thirdparty_suppress_warnings()

add_module( "lib" "SV_RCon_Pb" "vpc" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "sv_rcon.pb.cc"
    "sv_rcon.pb.h"
)

end_sources()
thirdparty_suppress_warnings()

add_module( "lib" "CL_RCon_Pb" "vpc" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "cl_rcon.pb.cc"
    "cl_rcon.pb.h"
)

end_sources()
thirdparty_suppress_warnings()
