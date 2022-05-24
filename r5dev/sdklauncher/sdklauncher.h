#pragma once
#include "basepanel.h"

class CLauncher
{
public:
    CLauncher()
    {
        m_svCurrentDir = fs::current_path().u8string();
        //m_pMainUI = new CUIBasePanel();
    }
    ~CLauncher()
    {
        delete[] m_pMainUI;
    }

    bool Setup(eLaunchMode lMode, eLaunchState lState);
    bool Setup(eLaunchMode lMode, const string& svCommandLine);
    bool Launch();
    CUIBasePanel* GetMainSurface() const { return m_pMainUI; }

    CUIBasePanel* m_pMainUI;

private:

    string m_svWorkerDll;
    string m_svGameExe;
    string m_svCmdLine;
    string m_svCurrentDir;
};
inline CLauncher* g_pLauncher = new CLauncher();