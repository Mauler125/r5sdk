# -----------------------------------------------------------------------------
# Setup each build configuration
# -----------------------------------------------------------------------------
macro( apply_project_settings )
    # Set common settings for all configurations
    add_compile_options(
        $<$<CXX_COMPILER_ID:MSVC>:/permissive->
        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/Zf>
        $<$<CXX_COMPILER_ID:MSVC>:/Zi>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:MSVC>:/GR>
    )

    # Suppress certain compiler warnings
    # ( !don't suppress warnings here that are specific to a project! )
    add_compile_options(
        $<$<CXX_COMPILER_ID:MSVC>:/wd4996> # 'The POSIX name for this item is deprecated'
        $<$<CXX_COMPILER_ID:MSVC>:/wd4127> # 'Consider using 'if constexpr' statement instead'
    )

    # Some thirdparty code have Warnings as Errors disabled; this option won't override those.
    option( OPTION_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON )

    set( OPTION_LTCG_MODE "OFF" CACHE STRING "Enables link-time code generation (significantly increases compile times)" )
    set_property( CACHE OPTION_LTCG_MODE PROPERTY STRINGS
    "OFF"
    "ON"  # Only on projects that specified LTCG
    "ALL" # All projects, whether or not LTCG was specified
    )

    option( OPTION_CERTAIN "This build is certain; debug statements (such as DevMsg(...)) will NOT be compiled" OFF )
    option( OPTION_RETAIL "This build is retail; enable this among with 'OPTION_CERTAIN' to form a release build" OFF )

    # Set common defines
    add_compile_definitions(
        "_CRT_SECURE_NO_WARNINGS"
        "SPDLOG_COMPILED_LIB"
        "SPDLOG_NO_EXCEPTIONS"
        "CURL_STATICLIB"

        # Must be explicitly defined to toggle SIMD optimizations for RapidJSON.
        # Don't set this to anything higher than SSE2, as the game supports from
        # SSE3 and higher, and the next level of optimizations in RapidJSON is SSE4.2.
        "RAPIDJSON_SSE2"

        # Use iterative parsing to protect against stack overflows in rare cases; see:
        # https://rapidjson.org/md_doc_features.html
        # https://github.com/Tencent/rapidjson/issues/1227
        # https://github.com/Tencent/rapidjson/issues/2260
        "RAPIDJSON_PARSE_DEFAULT_FLAGS=kParseIterativeFlag|kParseValidateEncodingFlag"

        # Target is 64bits only.
        "PLATFORM_64BITS"
    )

    if( ${OPTION_CERTAIN} )
    add_compile_definitions(
        "_CERT"
    )
    endif()

    if( ${OPTION_RETAIL} )
    add_compile_definitions(
        "_RETAIL"
    )
    endif()

    # Set settings for Debug configuration
    add_compile_options(
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/MTd>
    )

    # Set settings for Profile configuration
    add_compile_options(
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Profile>>:/Ox>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Profile>>:/GF>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Profile>>:/MT>
    )
    set( CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_PROFILE} /PROFILE" )

    # Set settings for Release configuration
    add_compile_options(
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/GF>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/MT>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/EHsc>
    )

    if( ${OPTION_LTCG_MODE} STREQUAL "ALL" )
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_PROFILE ON)
    endif()

    set( CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}
        /OPT:REF
        /OPT:ICF
        /RELEASE
        /SAFESEH:NO
        /DEBUG"
    )

    # Commonly used directories across libraries
    include_directories(
        "${ENGINE_SOURCE_DIR}/"
        "${ENGINE_SOURCE_DIR}/public/"
        "${THIRDPARTY_SOURCE_DIR}/"
        "${THIRDPARTY_SOURCE_DIR}/imgui/"
        "${THIRDPARTY_SOURCE_DIR}/recast/"
    )
endmacro()

# -----------------------------------------------------------------------------
# Setup build output directories for target
# -----------------------------------------------------------------------------
macro( set_target_output_dirs TARGET RUNTIME_DIR )
    # Set output directories
    set_target_properties( ${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${RUNTIME_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/${RUNTIME_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${RUNTIME_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_PROFILE "${CMAKE_SOURCE_DIR}/${RUNTIME_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${TARGET}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${TARGET}"
    )

    # Set output directories for each configuration
    foreach( CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES} )
        set_target_properties( ${TARGET} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE}" "${CMAKE_SOURCE_DIR}/${RUNTIME_DIR}"
            "ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE}" "${CMAKE_SOURCE_DIR}/lib/${TARGET}/${CONFIG_TYPE}"
            "LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE}" "${CMAKE_SOURCE_DIR}/lib/${TARGET}/${CONFIG_TYPE}"
            "LINK_FLAGS_${CONFIG_TYPE}" "/PDB:${PDB_FULL_PATH}"
        )
    endforeach()

    # Set PDB properties for release builds ( should be created )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi" )
    set( PDB_OUTPUT_DIRECTORY "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE}" )

    # Set linker properties
    set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF" )
    set( CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CMAKE_SHARED_LINKER_FLAGS_PROFILE} /DEBUG /PROFILE" )
endmacro()
