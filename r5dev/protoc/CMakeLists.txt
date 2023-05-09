cmake_minimum_required( VERSION 3.16 )

project( protocol_pb )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "NetCon"
    "cl_rcon.pb.cc"
    "cl_rcon.pb.h"
    "sv_rcon.pb.cc"
    "sv_rcon.pb.h"
)

add_sources( SOURCE_GROUP "SigCache"
    "sig_map.pb.cc"
    "sig_map.pb.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
