#pragma once

#define MAIN_WORKER_DLL "gamesdk.dll"
#define SERVER_WORKER_DLL "dedicated.dll"
#define CLIENT_WORKER_DLL "bin\\x64_retail\\client.dll"

#define MAIN_GAME_DLL "r5apex.exe"
#define SERVER_GAME_DLL "r5apex_ds.exe"

#define GAME_CFG_PATH "platform\\cfg\\"

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
