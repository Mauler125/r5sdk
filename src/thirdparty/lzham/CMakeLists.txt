cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "liblzham" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
	"lzham_api.cpp"
	"lzham_assert.cpp"
	"lzham_checksum.cpp"
	"lzham_huffman_codes.cpp"
	"lzham_lzbase.cpp"
	"lzham_match_accel.cpp"
	"lzham_mem.cpp"
	"lzham_platform.cpp"
	"lzham_polar_codes.cpp"
	"lzham_prefix_coding.cpp"
	"lzham_pthreads_threading.cpp"
	"lzham_symbol_codec.cpp"
	"lzham_timer.cpp"
	"lzham_vector.cpp"
	"lzham_win32_threading.cpp"
)

add_sources( SOURCE_GROUP "Compress"
	"lzhamcomp/lzham_comp.h"
	"lzhamcomp/lzham_lzcomp.cpp"
	"lzhamcomp/lzham_lzcomp_internal.cpp"
	"lzhamcomp/lzham_lzcomp_internal.h"
	"lzhamcomp/lzham_lzcomp_state.cpp"
)

add_sources( SOURCE_GROUP "Decompress"
	"lzhamdecomp/lzham_decomp.h"
	"lzhamdecomp/lzham_lzdecomp.cpp"
	"lzhamdecomp/lzham_lzdecompbase.cpp"
	"lzhamdecomp/lzham_lzdecompbase.h"
)

add_sources( SOURCE_GROUP "Include"
	"include/lzham.h"
	"include/lzham_assert.h"
	"include/lzham_checksum.h"
	"include/lzham_config.h"
	"include/lzham_core.h"
	"include/lzham_helpers.h"
	"include/lzham_huffman_codes.h"
	"include/lzham_lzbase.h"
	"include/lzham_match_accel.h"
	"include/lzham_math.h"
	"include/lzham_mem.h"
	"include/lzham_null_threading.h"
	"include/lzham_platform.h"
	"include/lzham_polar_codes.h"
	"include/lzham_prefix_coding.h"
	"include/lzham_pthreads_threading.h"
	"include/lzham_static_lib.h"
	"include/lzham_symbol_codec.h"
	"include/lzham_threading.h"
	"include/lzham_timer.h"
	"include/lzham_traits.h"
	"include/lzham_types.h"
	"include/lzham_utils.h"
	"include/lzham_vector.h"
	"include/lzham_win32_threading.h"
)

end_sources()
whole_program_optimization()

target_compile_definitions( ${PROJECT_NAME} PRIVATE
	"WIN32"
)
