cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libjwt" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
	"base64.cpp"
	"claim.cpp"
	"decode.cpp"
	"encode.cpp"
	"util.cpp"
	"version.cpp"
)

add_sources( SOURCE_GROUP "Include"
	"include/algs.h"
	"include/base64.h"
	"include/claim.h"
	"include/decode.h"
	"include/encode.h"
	"include/retcodes.h"
	"include/util.h"
	"include/version.h"
)

end_sources()
thirdparty_suppress_warnings()
