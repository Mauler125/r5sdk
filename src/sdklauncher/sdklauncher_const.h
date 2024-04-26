#pragma once

// Change this each time the settings format has changed.
#define SDK_LAUNCHER_VERSION 3

// Uncomment this line to compile the launcher for dedicated server builds.
//#define DEDI_LAUNCHER

#define GAME_CFG_PATH "platform\\cfg\\system\\"
#define DEFAULT_WINDOW_CLASS_NAME "Respawn001"
#define LAUNCHER_SETTING_FILE "launcher.vdf"

//-----------------------------------------------------------------------------
// Launch and inject specified dll based on launch mode
//-----------------------------------------------------------------------------
enum class eLaunchMode : int
{
    LM_NONE = -1,
    LM_GAME_DEV,
    LM_GAME,
    LM_SERVER_DEV,
    LM_SERVER,
    LM_CLIENT_DEV,
    LM_CLIENT,
};
