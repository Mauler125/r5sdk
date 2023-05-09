
# -----------------------------------------------------------------------------
# Set global configuration types
# -----------------------------------------------------------------------------
macro( setup_build_configurations )
set( CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release" CACHE STRING "" FORCE )
endmacro()
