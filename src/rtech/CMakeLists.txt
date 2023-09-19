cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "rtech_game" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
   "rtech_game.cpp"
   "rtech_game.h"
)

add_sources( SOURCE_GROUP "Public"
   "${ENGINE_SOURCE_DIR}/public/rtech/ipakfile.h"
)

end_sources()

add_module( "lib" "rtech_tools" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
   "rtech_utils.cpp"
   "rtech_utils.h"
)

add_sources( SOURCE_GROUP "Public"
   "${ENGINE_SOURCE_DIR}/public/rtech/ipakfile.h"
)

end_sources()

add_module( "lib" "rui" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
   "rui/rui.cpp"
   "rui/rui.h"
)

end_sources()

add_module( "lib" "stryder" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
   "stryder/stryder.cpp"
   "stryder/stryder.h"
)

end_sources()
