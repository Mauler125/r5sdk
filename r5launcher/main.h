#pragma once

//-----------------------------------------------------------------------------
// Launch and inject specified dll based on launchmode
//-----------------------------------------------------------------------------
enum class LAUNCHMODE : int
{
    LM_NULL,
    LM_DEBUG,  // Debug DLL
    LM_GAME,  // Release DLL
    LM_DEDI  // Dedi DLL
};

//-----------------------------------------------------------------------------
// [TODO] Launch with FCVAR_DEVELOPMENTONLY and FCVAR_CHEATS disabled/enabled
//-----------------------------------------------------------------------------
enum class LAUNCHSTATE : int
{
    LS_NOCHEATS, // Disabled cheats
    LS_CHEATS,   // Enable cheats
    LS_DEBUG     // Enable debug
};