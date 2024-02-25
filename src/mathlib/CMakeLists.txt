cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "mathlib" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Hash"
    "adler32.cpp"
    "adler32.h"
    "crc32.cpp"
    "crc32.h"
    "IceKey.cpp"
    "IceKey.H"
    "sha1.cpp"
    "sha1.h"
)

add_sources( SOURCE_GROUP "RNG"
    "halton.cpp"
    "halton.h"
    "randsse.cpp"
    "ssenoise.cpp"
)

add_sources( SOURCE_GROUP "Vector"
    "bitvec.h"
    "color.h"
    "color_conversion.cpp"
    "compressed_vector.h"
    "fltx4.h"
    "ssemath.h"
    "ssequaternion.h"
    "transform.cpp"
    "transform.h"
    "vector.h"
    "vector2d.h"
    "vector4d.h"
    "vmatrix.cpp"
    "vmatrix.h"
    "vplane.h"
)

add_sources( SOURCE_GROUP "Math"
    "almostequal.cpp"
    "bits.h"
    "fbits.h"
    "math_pfns.h"
    "mathlib.h"
    "mathlib_base.cpp"
    "noisedata.h"
    "parallel_for.h"
    "powsse.cpp"
    "sseconst.cpp"
    "ssemath.h"
    "swap.h"
)

end_sources()
whole_program_optimization()

target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "mathlib_pch.h"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "BUILDING_MATHLIB"
    "ALLOW_SIMD_QUATERNION_MATH"
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:Profile>>:DEBUG_MATHLIB>
)
