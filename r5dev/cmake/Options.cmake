# -----------------------------------------------------------------------------
# Setup each build configuration
# -----------------------------------------------------------------------------
macro( apply_project_settings )
    # Set common settings for all configurations
    add_compile_options(
        $<$<CXX_COMPILER_ID:MSVC>:/permissive->
        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/Zf>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:MSVC>:/GR>
        $<$<CXX_COMPILER_ID:MSVC>:/D_UNICODE>
        $<$<CXX_COMPILER_ID:MSVC>:/DUNICODE>
    )

    # Suppress certain compiler warnings
    # ( !don't suppress warnings here that are specific to a project! )
    add_compile_options(
        $<$<CXX_COMPILER_ID:MSVC>:/wd4996> # 'The POSIX name for this item is deprecated'
        $<$<CXX_COMPILER_ID:MSVC>:/wd4127> # 'Consider using 'if constexpr' statement instead'
    )

    # Some thirdparty code have Warnings as Errors disabled; this option won't override those.
    option( GLOBAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON )

    set( GAMEDLL_OPTION "GAMEDLL_S3" CACHE STRING "Game DLL version" )
    set_property( CACHE GAMEDLL_OPTION PROPERTY STRINGS
        "GAMEDLL_S0"
        "GAMEDLL_S1"
        "GAMEDLL_S2"
        "GAMEDLL_S3"
    )

    # Set common defines
    add_compile_definitions(
        "_CRT_SECURE_NO_WARNINGS"
        "SPDLOG_COMPILED_LIB"
        "SPDLOG_NO_EXCEPTIONS"
        "CURL_STATICLIB"
        "${GAMEDLL_OPTION}"
    )

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
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Ob2>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Oi>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Ot>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/GF>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/MT>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/GS->
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Gy>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/EHsc>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/fp:fast>
    )

    set( CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}
        /OPT:REF
        /OPT:ICF
        /RELEASE
        /SAFESEH:NO
        /DEBUG"
    )

    # Commonly used directories accross libraries
    include_directories(
        "${ENGINE_SOURCE_DIR}/"
        "${ENGINE_SOURCE_DIR}/public/"
        "${ENGINE_SOURCE_DIR}/thirdparty/"
        "${ENGINE_SOURCE_DIR}/thirdparty/imgui/"
        "${ENGINE_SOURCE_DIR}/thirdparty/recast/"
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
