cmake_minimum_required( VERSION 3.16 )

project( vscript )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "vscript.cpp"
    "vscript.h"
)

add_sources( SOURCE_GROUP "Squirrel_RE"
    "languages/squirrel_re/vsquirrel.cpp"
    "languages/squirrel_re/vsquirrel.h"
    "languages/squirrel_re/include/sqstate.h"
    "languages/squirrel_re/include/sqstdaux.h"
    "languages/squirrel_re/include/squirrel.h"
    "languages/squirrel_re/include/sqvm.h"

    "languages/squirrel_re/sqstdlib/sqstdaux.cpp"
    "languages/squirrel_re/squirrel/sqapi.cpp"
    "languages/squirrel_re/squirrel/sqvm.cpp"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/vscript/ivscript.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
