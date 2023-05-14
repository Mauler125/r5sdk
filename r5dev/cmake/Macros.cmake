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
# End the source file list ( optional parameter sets the runtime output dir )
# -----------------------------------------------------------------------------
macro( end_sources )
    if( NOT "${ARGN}" STREQUAL "" ) # Check if an output directory is passed
        set_target_output_dirs( ${PROJECT_NAME} ${ARGN} )
    else()
        set_target_output_dirs( ${PROJECT_NAME} "game/" )
    endif()
endmacro()

# -----------------------------------------------------------------------------
# Add modules to the project
# -----------------------------------------------------------------------------
macro( add_module MODULE_TYPE MODULE_NAME REUSE_PCH FOLDER_NAME )
    project( ${MODULE_NAME} )

    if( ${MODULE_TYPE} STREQUAL "lib" )
        add_library( ${PROJECT_NAME} )
    elseif( ${MODULE_TYPE} STREQUAL "shared_lib" )
        add_library( ${PROJECT_NAME} SHARED )
        target_link_options( ${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Release>:/LTCG>"
    )
    elseif(${MODULE_TYPE} STREQUAL "exe")
        add_executable( ${PROJECT_NAME} )
        target_link_options( ${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Release>:/LTCG>"
    )
    else()
        message( FATAL_ERROR "Invalid module type: ${MODULE_TYPE}; expected 'lib', 'shared_lib', or 'exe'." )
    endif()

    if ( NOT "${REUSE_PCH}" STREQUAL "" )
        target_precompile_headers( ${PROJECT_NAME} REUSE_FROM ${REUSE_PCH} )
    endif()

    set_target_properties( ${MODULE_NAME} PROPERTIES FOLDER ${FOLDER_NAME} )
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

# -----------------------------------------------------------------------------
# Apply whole program optimization for this target in release ( !slow! )
# -----------------------------------------------------------------------------
macro( whole_program_optimization )
    target_compile_options( ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/GL>
    )
endmacro()
