cmake_minimum_required( VERSION 3.16 )
add_module( "exe" "recast" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Builder"
    "Editor_Common.cpp"
    "Editor_SoloMesh.cpp"
    "Editor_TileMesh.cpp"
    "InputGeom.cpp"
)

add_sources( SOURCE_GROUP "Builder/Include"
    "Include/Editor_Common.h"
    "Include/Editor_SoloMesh.h"
    "include/Editor_TileMesh.h"
    "include/InputGeom.h"
)

add_sources( SOURCE_GROUP "Core"
    "Editor.cpp"
    "main.cpp"
    "Pch.cpp"
)

add_sources( SOURCE_GROUP "Core/Include"
    "include/Editor.h"
    "include/Pch.h"
)

add_sources( SOURCE_GROUP "IO"
    "Filelist.cpp"
    "MeshLoaderBsp.cpp"
    "MeshLoaderObj.cpp"
    "MeshLoaderPly.cpp"
)

add_sources( SOURCE_GROUP "IO/Include"
    "include/Filelist.h"
    "include/FileTypes.h"
    "include/MeshLoaderBsp.h"
    "include/MeshLoaderObj.h"
    "include/MeshLoaderPly.h"
)

add_sources( SOURCE_GROUP "Tools"
    "ChunkyTriMesh.cpp"
    "ConvexVolumeTool.cpp"
    "CrowdTool.cpp"
    "NavMeshPruneTool.cpp"
    "NavMeshTesterTool.cpp"
    "OffMeshConnectionTool.cpp"
)

add_sources( SOURCE_GROUP "Tools/Include"
    "include/ChunkyTriMesh.h"
    "include/ConvexVolumeTool.h"
    "include/CrowdTool.h"
    "include/NavMeshPruneTool.h"
    "include/NavMeshTesterTool.h"
    "include/OffMeshConnectionTool.h"
)

add_sources( SOURCE_GROUP "Utils"
    "Editor_Debug.cpp"
    "EditorInterfaces.cpp"
    "GameUtils.cpp"
    "PerfTimer.cpp"
    "TestCase.cpp"
    "ValueHistory.cpp"
)

add_sources( SOURCE_GROUP "Utils/Include"
    "include/Editor_Debug.h"
    "include/EditorInterfaces.h"
    "include/GameUtils.h"
    "include/PerfTimer.h"
    "include/TestCase.h"
    "include/ValueHistory.h"
)

add_sources( SOURCE_GROUP "Resource"
    "Include/Icon.h"
    "Include/DroidSans.h"
)

end_sources( "${BUILD_OUTPUT_DIR}/bin/" )
whole_program_optimization()

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS "-console"
    VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)../../../${BUILD_OUTPUT_DIR}/bin/"
)
target_compile_definitions( ${PROJECT_NAME} PRIVATE
    "WIN32"
    "_TOOLS"
)
target_precompile_headers( ${PROJECT_NAME} PRIVATE
    "Include/Pch.h"
)
target_link_libraries( ${PROJECT_NAME} PRIVATE
    "navsharedcommon"
    "navdebugutils"
    "libdetour"
    "libdetourcrowd"
    "libdetourtilecache"
    "librecast"
    "libsdl2"
    "libimgui"
    "FastLZ"
    "Rpcrt4.lib"
    "ws2_32.lib"
    "winmm.lib"
    "imm32.lib"
    "version.lib"
    "setupapi.lib"
    "OpenGL32.lib"
    "Glu32.lib"
)
