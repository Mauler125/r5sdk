#pragma once

//-----------------------------------------------------------------------------
// Launch and inject specified dll based on launchmode
//-----------------------------------------------------------------------------
enum class eLaunchMode : int
{
    LM_NULL,
    LM_DEBUG_GAME,   // Debug worker DLL.
    LM_RELEASE_GAME, // Release worker DLL.
    LM_DEBUG_DEDI,   // Debug dedicated DLL.
    LM_RELEASE_DEDI  // Release dedicated DLL.
};

//-----------------------------------------------------------------------------
// [TODO] Launch with FCVAR_DEVELOPMENTONLY and FCVAR_CHEATS disabled/enabled
//-----------------------------------------------------------------------------
enum class eLaunchState : int
{
    LS_NULL,
    LS_NOCHEATS, // Disabled cheats
    LS_CHEATS,   // Enable cheats
    LS_DEBUG     // Enable debug
};
