#pragma once

//-----------------------------------------------------------------------------
// Launch and inject specified dll based on launch mode
//-----------------------------------------------------------------------------
enum class eLaunchMode : int
{
    LM_NONE = -1,
    LM_HOST_DEV,
    LM_HOST,
    LM_SERVER_DEV,
    LM_SERVER,
    LM_CLIENT_DEV,
    LM_CLIENT,
};

//-----------------------------------------------------------------------------
// [TODO] Launch with FCVAR_DEVELOPMENTONLY and FCVAR_CHEATS disabled/enabled
//-----------------------------------------------------------------------------
enum class eLaunchState : int
{
    LS_NULL,
    LS_NOCHEATS, // Disabled cheats
    LS_CHEATS,   // Enable cheats
    LS_DEV       // Enable devonly
};