#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class CLauncher
{
public:
    CLauncher(const char* pszLoggerName)
    {
		m_pSurface = nullptr;
		m_pLogger = spdlog::stdout_color_mt(pszLoggerName);
        m_ProcessorAffinity = NULL;
		m_svCurrentDir = fs::current_path().u8string();
    }
    ~CLauncher()
    {
		if (m_pSurface)
		{
			delete m_pSurface;
		}
    }

    void AddLog(spdlog::level::level_enum nLevel, const char* szFormat, ...)
    {
        string svBuffer;
        va_list args;
        va_start(args, szFormat);
        svBuffer = FormatV(szFormat, args);
        va_end(args);

        m_pLogger->log(nLevel, svBuffer);
        m_pLogger->flush();

        if (m_pSurface)
        {
            m_pSurface->m_LogList.push_back(LogList_t(nLevel, svBuffer));
            m_pSurface->ConsoleListView()->SetVirtualListSize(static_cast<int32_t>(m_pSurface->m_LogList.size()));
            m_pSurface->ConsoleListView()->Refresh();
        }
    }

    void RunSurface();
    void InitConsole();
    void InitLogger();

    int HandleCommandLine(int argc, char* argv[]);
    int HandleInput();

    bool CreateLaunchContext(eLaunchMode lMode, uint64_t nProcessorAffinity = NULL, const char* szCommandLine = nullptr, const char* szConfig = nullptr);
    void SetupLaunchContext(const char* szConfig, const char* szGameDll, const char* szCommandLine);
    bool LaunchProcess() const;

    CSurface* GetMainSurface() const { return m_pSurface; }

private:
    CSurface* m_pSurface;
	std::shared_ptr<spdlog::logger> m_pLogger;

    uint64_t m_ProcessorAffinity;

    string m_svGameDll;
    string m_svCmdLine;
    string m_svCurrentDir;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

extern CLauncher* g_pLauncher;
