cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libzstd" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Common"
	"common/allocations.h"
	"common/bits.h"
	"common/bitstream.h"
	"common/compiler.h"
	"common/cpu.h"
	"common/debug.c"
	"common/debug.h"
	"common/entropy_common.c"
	"common/error_private.c"
	"common/error_private.h"
	"common/fse.h"
	"common/fse_decompress.c"
	"common/huf.h"
	"common/mem.h"
	"common/pool.c"
	"common/pool.h"
	"common/portability_macros.h"
	"common/threading.c"
	"common/threading.h"
	"common/xxhash.c"
	"common/xxhash.h"
	"common/zstd_common.c"
	"common/zstd_deps.h"
	"common/zstd_internal.h"
	"common/zstd_trace.h"
)

add_sources( SOURCE_GROUP "Compress"
	"compress/clevels.h"
	"compress/fse_compress.c"
	"compress/hist.c"
	"compress/hist.h"
	"compress/huf_compress.c"
	"compress/zstdmt_compress.c"
	"compress/zstdmt_compress.h"
	"compress/zstd_compress.c"
	"compress/zstd_compress_internal.h"
	"compress/zstd_compress_literals.c"
	"compress/zstd_compress_literals.h"
	"compress/zstd_compress_sequences.c"
	"compress/zstd_compress_sequences.h"
	"compress/zstd_compress_superblock.c"
	"compress/zstd_compress_superblock.h"
	"compress/zstd_cwksp.h"
	"compress/zstd_double_fast.c"
	"compress/zstd_double_fast.h"
	"compress/zstd_fast.c"
	"compress/zstd_fast.h"
	"compress/zstd_lazy.c"
	"compress/zstd_lazy.h"
	"compress/zstd_ldm.c"
	"compress/zstd_ldm.h"
	"compress/zstd_ldm_geartab.h"
	"compress/zstd_opt.c"
	"compress/zstd_opt.h"
)

add_sources( SOURCE_GROUP "Decompress"
	"decompress/huf_decompress.c"
	"decompress/huf_decompress_amd64.S"
	"decompress/zstd_ddict.c"
	"decompress/zstd_ddict.h"
	"decompress/zstd_decompress.c"
	"decompress/zstd_decompress_block.c"
	"decompress/zstd_decompress_block.h"
	"decompress/zstd_decompress_internal.h"
)

add_sources( SOURCE_GROUP "DictBuilder"
	"dictBuilder/cover.c"
	"dictBuilder/cover.h"
	"dictBuilder/divsufsort.c"
	"dictBuilder/divsufsort.h"
	"dictBuilder/fastcover.c"
	"dictBuilder/zdict.c"
)

add_sources( SOURCE_GROUP "Include"
	"zdict.h"
	"zstd.h"
	"zstd_errors.h"
)

end_sources()

whole_program_optimization()
thirdparty_suppress_warnings()

target_compile_definitions( ${PROJECT_NAME} PRIVATE
	"ZSTD_MULTITHREAD"
	"ZSTD_NO_FORWARD_PROGRESS_MAX=1024"
	"DEBUGLEVEL=0"
)
