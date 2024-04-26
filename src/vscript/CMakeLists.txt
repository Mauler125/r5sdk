cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vscript" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "vscript.cpp"
    "vscript.h"
)

add_sources( SOURCE_GROUP "Squirrel_RE"
    "languages/squirrel_re/vsquirrel.cpp"
    "languages/squirrel_re/vsquirrel.h"
)

add_sources( SOURCE_GROUP "Squirrel_RE/squirrel"
    "languages/squirrel_re/squirrel/sqapi.cpp"
    "languages/squirrel_re/squirrel/sqdebug.cpp"
    #"languages/squirrel_re/squirrel/sqcompiler.cpp"
    "languages/squirrel_re/squirrel/sqfuncstate.cpp"
    #"languages/squirrel_re/squirrel/sqlexer.cpp"
    "languages/squirrel_re/squirrel/sqobject.cpp"
    "languages/squirrel_re/squirrel/sqstring.cpp"
    "languages/squirrel_re/squirrel/sqtable.cpp"
    "languages/squirrel_re/squirrel/sqmem.cpp"
    "languages/squirrel_re/squirrel/sqstate.cpp"
    "languages/squirrel_re/squirrel/sqvm.cpp"
)

add_sources( SOURCE_GROUP "Squirrel_RE/sqstdlib"
    "languages/squirrel_re/sqstdlib/sqstdaux.cpp"
)

add_sources( SOURCE_GROUP "Squirrel_RE/include"
    "languages/squirrel_re/vsquirrel.cpp"
    "languages/squirrel_re/vsquirrel.h"
    #"languages/squirrel_re/include/sqcompiler.h"
    "languages/squirrel_re/include/sqfuncstate.h"
    #"languages/squirrel_re/include/sqlexer.h"
    "languages/squirrel_re/include/sqarray.h"
    "languages/squirrel_re/include/squtils.h"
    "languages/squirrel_re/include/sqobject.h"
    "languages/squirrel_re/include/sqopcodes.h"
    "languages/squirrel_re/include/sqstate.h"
    "languages/squirrel_re/include/sqstdaux.h"
    "languages/squirrel_re/include/sqstring.h"
    "languages/squirrel_re/include/sqtable.h"
    "languages/squirrel_re/include/squirrel.h"
    "languages/squirrel_re/include/sqvm.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/vscript/ivscript.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/vscript/languages/squirrel_re/include/" )
