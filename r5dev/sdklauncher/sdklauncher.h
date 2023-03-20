#pragma once
#include "basepanel.h"
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class CLauncher
{
public:
    CLauncher(const char* pszLoggerName)
    {
		m_pSurface = nullptr;
		m_pLogger = spdlog::stdout_color_mt(pszLoggerName);
		m_svCurrentDir = fs::current_path().u8string();
    }
    ~CLauncher()
    {
		if (m_pSurface)
		{
			delete m_pSurface;
		}
    }
    template <typename T, typename ...P>
    void AddLog(spdlog::level::level_enum nLevel, T&& svFormat, P &&... vParams)
    {
        String svBuffer = fmt::format(std::forward<T>(svFormat), std::forward<P>(vParams)...).c_str();
        m_pLogger->log(nLevel, svBuffer.ToCString());
        m_pLogger->flush();

        if (m_pSurface)
        {
            m_pSurface->m_LogList.push_back(LogList_t(nLevel, svBuffer));
            m_pSurface->m_ConsoleListView->SetVirtualListSize(static_cast<int32_t>(m_pSurface->m_LogList.size()));
            m_pSurface->m_ConsoleListView->Refresh();
        }
    }

    void RunSurface();
    void InitConsole();
    void InitLogger();

    int HandleCommandLine(int argc, char* argv[]);
    int HandleInput();

    bool CreateLaunchContext(eLaunchMode lMode, const char* szCommandLine = nullptr, const char* szConfig = nullptr);
    void SetupLaunchContext(const char* szConfig, const char* szWorkerDll, const char* szGameDll, const char* szCommandLine);
    bool LaunchProcess() const;

    CUIBaseSurface* GetMainSurface() const { return m_pSurface; }

private:
    CUIBaseSurface* m_pSurface;
	std::shared_ptr<spdlog::logger> m_pLogger;

    string m_svWorkerDll;
    string m_svGameExe;
    string m_svCmdLine;
    string m_svCurrentDir;
};

extern CLauncher* g_pLauncher;
