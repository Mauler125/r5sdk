# -----------------------------------------------------------------------------
# Creates and writes the build string after building a project
# -----------------------------------------------------------------------------
function( WriteBuildString OUTPUT_DIR )
    # Get the current date and time
    string( TIMESTAMP CURRENT_DATE "%Y_%m_%d_%I_%M" )  # Use %I for 12-hour clock

    # Compute AM/PM
    string( TIMESTAMP CURRENT_HOUR "%H" )
    if( CURRENT_HOUR LESS 12 )
        set( TIME_DESIGNATOR "AM" )
    else()
        set( TIME_DESIGNATOR "PM" )
    endif()

    # Get the current git commit hash
    execute_process( COMMAND
        git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Convert the hash to uppercase
    string( TOUPPER
        "${GIT_COMMIT_HASH}" GIT_COMMIT_HASH
    )

    # Get the current git branch name
    execute_process( COMMAND
        git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Construct the build string
    set( BUILD_STRING
        "R5pc_${GIT_BRANCH_NAME}_N1094_${GIT_COMMIT_HASH}_${CURRENT_DATE}_${TIME_DESIGNATOR}\n"
    )

    # Write the build string to a file
    file( WRITE
        "${CMAKE_SOURCE_DIR}/${OUTPUT_DIR}/build.txt" "${BUILD_STRING}"
    )
endfunction()

# Initiate the creation command
WriteBuildString( "../../../game" )
