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

class CLauncher
{
public:
    CLauncher()
    {
        m_svCurrentDir = fs::current_path().u8string();
    }

    bool Setup(eLaunchMode lMode, eLaunchState lState);
    bool Launch();

private:
    string m_svWorkerDll;
    string m_svGameExe;
    string m_svCmdLine;
    string m_svCurrentDir;
};
inline CLauncher* g_pLauncher = new CLauncher();