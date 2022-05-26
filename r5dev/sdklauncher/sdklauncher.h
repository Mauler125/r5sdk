#pragma once
#include "basepanel.h"
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class CLauncher
{
public:
    CLauncher()
    {
        m_svCurrentDir = fs::current_path().u8string();
    }
    ~CLauncher()
    {
        delete[] m_pSurface;
    }
    template <typename T, typename ...P>
    void AddLog(spdlog::level::level_enum nLevel, T&& svFormat, P &&... vParams)
    {
        String svBuffer = fmt::format(std::forward<T>(svFormat), std::forward<P>(vParams)...).c_str();
        wconsole->log(nLevel, svBuffer.ToCString());
        wconsole->flush();

        if (m_pSurface)
        {
            m_pSurface->m_LogList.push_back(LogList_t(nLevel, svBuffer));
            m_pSurface->m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(m_pSurface->m_LogList.size()));
            m_pSurface->m_ConsoleListView->Refresh();
        }
    }

    void InitSurface();
    void InitConsole();
    void InitLogger();
    int HandleCmdLine(int argc, char* argv[]);
    int HandleInput();

    bool Setup(eLaunchMode lMode, eLaunchState lState);
    bool Setup(eLaunchMode lMode, const string& svCommandLine);
    bool Launch() const;

    CUIBaseSurface* GetMainSurface() const { return m_pSurface; }

private:
    CUIBaseSurface* m_pSurface = nullptr;
    std::shared_ptr<spdlog::logger> wconsole = spdlog::stdout_color_mt("win_console");

    string m_svWorkerDll;
    string m_svGameExe;
    string m_svCmdLine;
    string m_svCurrentDir;
};
inline CLauncher* g_pLauncher = new CLauncher();