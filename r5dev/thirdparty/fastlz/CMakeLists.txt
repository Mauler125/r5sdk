cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "FastLZ" "" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
   "fastlz.c"
   "fastlz.h"
)

end_sources()
