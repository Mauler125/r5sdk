#pragma once

// Change this each time the settings format has changed.
#define SDK_LAUNCHER_VERSION 1

// Uncomment this line to compile the launcher for dedicated server builds.
//#define DEDI_LAUNCHER

#define GAME_CFG_PATH "platform\\cfg\\system\\"
#define DEFAULT_WINDOW_CLASS_NAME "Respawn001"
#define LAUNCHER_SETTING_FILE "launcher.vdf"

// Gigabytes.
// TODO: obtain these from the repo...
#define MIN_REQUIRED_DISK_SPACE 20
#define MIN_REQUIRED_DISK_SPACE_OPT 55 // Optional textures

#define QUERY_TIMEOUT 15
#define BASE_PLATFORM_DIR "platform\\"
#define DEFAULT_DEPOT_DOWNLOAD_DIR BASE_PLATFORM_DIR "depot\\"

// Files that need to be installed during launcher restart,
// have to go here!!
#define RESTART_DEPOT_DOWNLOAD_DIR DEFAULT_DEPOT_DOWNLOAD_DIR "temp\\"

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
