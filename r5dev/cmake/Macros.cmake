# -----------------------------------------------------------------------------
# Start the source file list
# -----------------------------------------------------------------------------
macro( start_sources )
    add_sources( SOURCE_GROUP ""
        # Add the CMakeLists file to the project filter root
        "CMakeLists.txt"
)
endmacro()

# -----------------------------------------------------------------------------
# Add source files to target within a project filter
# -----------------------------------------------------------------------------
macro( add_sources )
    set( options )
    set( oneValueArgs SOURCE_GROUP )
    set( multiValueArgs )

    cmake_parse_arguments( ADD_SOURCES
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}" ${ARGN}
    )

    if( NOT ADD_SOURCES_SOURCE_GROUP )
        message( FATAL_ERROR "SOURCE_GROUP must be provided" )
    endif()

    source_group( "${ADD_SOURCES_SOURCE_GROUP}" FILES ${ADD_SOURCES_UNPARSED_ARGUMENTS} )
    target_sources( ${PROJECT_NAME} PRIVATE ${ADD_SOURCES_UNPARSED_ARGUMENTS} )
endmacro()

# -----------------------------------------------------------------------------
# End the source file list
# -----------------------------------------------------------------------------
macro( end_sources )
    get_property( SRCS_LIST GLOBAL PROPERTY SRCS_LIST )
    set_target_output_dirs( ${PROJECT_NAME} )
endmacro()

# -----------------------------------------------------------------------------
# Initialize global compiler defines
# -----------------------------------------------------------------------------
macro( define_compiler_variables )
    if( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
        add_definitions( -DCOMPILER_MSVC )
    elseif( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
        add_definitions( -DCOMPILER_CLANG )
    elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
        add_definitions( -DCOMPILER_GCC )
    else()
        message( FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}" )
    endif()
endmacro()
