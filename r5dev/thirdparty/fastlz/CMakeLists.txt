cmake_minimum_required( VERSION 3.16 )

project( FastLZ )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Core"
   "fastlz.c"
   "fastlz.h"
)

end_sources()
