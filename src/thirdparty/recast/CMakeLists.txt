cmake_minimum_required( VERSION 3.16 )

# -----------------------------------------------------------------------------
# Recast & Detour shared common
# -----------------------------------------------------------------------------
add_module( "lib" "navsharedcommon" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "Shared/Source/SharedAlloc.cpp"
    "Shared/Source/SharedAssert.cpp"
    "Shared/Source/SharedCommon.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "Shared/Include/SharedAlloc.h"
    "Shared/Include/SharedAssert.h"
    "Shared/Include/SharedCommon.h"
    "Shared/Include/SharedMath.h"
)

end_sources()
whole_program_optimization()

target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "Pch.h"
)

# -----------------------------------------------------------------------------
# Recast & Detour debug utilities
# -----------------------------------------------------------------------------
add_module( "lib" "navdebugutils" "navsharedcommon" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "DebugUtils/Source/DebugDraw.cpp"
    "DebugUtils/Source/DetourDebugDraw.cpp"
    "DebugUtils/Source/RecastDebugDraw.cpp"
    "DebugUtils/Source/RecastDump.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "DebugUtils/Include/DebugDraw.h"
    "DebugUtils/Include/DetourDebugDraw.h"
    "DebugUtils/Include/RecastDebugDraw.h"
    "DebugUtils/Include/RecastDump.h"
)

end_sources()
whole_program_optimization()

# -----------------------------------------------------------------------------
# Detour runtime
# -----------------------------------------------------------------------------
add_module( "lib" "libdetour" "navsharedcommon" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "Detour/Source/DetourNavMesh.cpp"
    "Detour/Source/DetourNavMeshBuilder.cpp"
    "Detour/Source/DetourNavMeshQuery.cpp"
    "Detour/Source/DetourNode.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "Detour/Include/DetourNavMesh.h"
    "Detour/Include/DetourNavMeshBuilder.h"
    "Detour/Include/DetourNavMeshQuery.h"
    "Detour/Include/DetourNode.h"
    "Detour/Include/DetourStatus.h"
)

end_sources()
whole_program_optimization()

# -----------------------------------------------------------------------------
# Detour crowd
# -----------------------------------------------------------------------------
add_module( "lib" "libdetourcrowd" "navsharedcommon" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "DetourCrowd/Source/DetourCrowd.cpp"
    "DetourCrowd/Source/DetourCrowdInternal.cpp"
    "DetourCrowd/Source/DetourLocalBoundary.cpp"
    "DetourCrowd/Source/DetourObstacleAvoidance.cpp"
    "DetourCrowd/Source/DetourPathCorridor.cpp"
    "DetourCrowd/Source/DetourPathQueue.cpp"
    "DetourCrowd/Source/DetourProximityGrid.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "DetourCrowd/Include/DetourCrowd.h"
    "DetourCrowd/Include/DetourCrowdInternal.h"
    "DetourCrowd/Include/DetourLocalBoundary.h"
    "DetourCrowd/Include/DetourObstacleAvoidance.h"
    "DetourCrowd/Include/DetourPathCorridor.h"
    "DetourCrowd/Include/DetourPathQueue.h"
    "DetourCrowd/Include/DetourProximityGrid.h"
)

end_sources()
whole_program_optimization()

# -----------------------------------------------------------------------------
# Detour tile cache
# -----------------------------------------------------------------------------
add_module( "lib" "libdetourtilecache" "navsharedcommon" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "DetourTileCache/Source/DetourTileCache.cpp"
    "DetourTileCache/Source/DetourTileCacheBuilder.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "DetourTileCache/Include/DetourTileCache.h"
    "DetourTileCache/Include/DetourTileCacheBuilder.h"
)

end_sources()
whole_program_optimization()

# -----------------------------------------------------------------------------
# Recast runtime
# -----------------------------------------------------------------------------
add_module( "lib" "librecast" "navsharedcommon" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Source"
    "Recast/Source/Recast.cpp"
    "Recast/Source/RecastArea.cpp"
    "Recast/Source/RecastContour.cpp"
    "Recast/Source/RecastFilter.cpp"
    "Recast/Source/RecastLayers.cpp"
    "Recast/Source/RecastMesh.cpp"
    "Recast/Source/RecastMeshDetail.cpp"
    "Recast/Source/RecastRasterization.cpp"
    "Recast/Source/RecastRegion.cpp"
)

add_sources( SOURCE_GROUP "Include"
    "Recast/Include/Recast.h"
)

end_sources()
whole_program_optimization()
