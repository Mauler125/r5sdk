cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "rtech" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Private"
   "rtech_game.cpp"
   "rtech_game.h"
   "rtech_utils.cpp"
   "rtech_utils.h"
)

add_sources( SOURCE_GROUP "RUI"
   "rui/rui.cpp"
   "rui/rui.h"
)

add_sources( SOURCE_GROUP "Stryder"
   "stryder/stryder.cpp"
   "stryder/stryder.h"
)

end_sources()
