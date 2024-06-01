cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "liblz4" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
	"lz4hc.c"
	"lz4frame.c"
	"lz4file.c"
	"lz4.c"
)

add_sources( SOURCE_GROUP "Include"
	"xxhash.h"
	"lz4hc.h"
	"lz4frame_static.h"
	"lz4frame.h"
	"lz4file.h"
	"lz4.h"
)

end_sources()

whole_program_optimization()
thirdparty_suppress_warnings()

# target_compile_definitions( ${PROJECT_NAME} PRIVATE
# )
