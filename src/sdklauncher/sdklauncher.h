#pragma once
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class CLauncher
{
public:
    CLauncher()
    {
        m_pSurface = nullptr;
        m_ProcessorAffinity = NULL;
		m_svCurrentDir = fs::current_path().u8string();
    }
    ~CLauncher()
    {
    }

    void RunSurface();

    void Init();
    void Shutdown();

    void AddLog(const LogType_t level, const char* szText);

    int HandleCommandLine(int argc, char* argv[]);
    int HandleInput();

    bool CreateLaunchContext(eLaunchMode lMode, uint64_t nProcessorAffinity = NULL, const char* szCommandLine = nullptr, const char* szConfig = nullptr);
    void SetupLaunchContext(const char* szConfig, const char* szGameDll, const char* szCommandLine);
    bool LaunchProcess() const;

    eLaunchMode BuildParameter(string& parameterList) { return m_pSurface->BuildParameter(parameterList); }

private:
    CSurface* m_pSurface;

    uint64_t m_ProcessorAffinity;

    string m_svGameDll;
    string m_svCmdLine;
    string m_svCurrentDir;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

extern CLauncher* SDKLauncher();
