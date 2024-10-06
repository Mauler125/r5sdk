#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H
#include "tier1/fmtstr.h"

#define CRASHMESSAGE_MSG_EXECUTABLE "bin\\crashmsg.exe"

class CCrashHandler;
typedef void (*CrashCallback_t)(const CCrashHandler* handler);

struct CrashHardWareInfo_s
{
	CrashHardWareInfo_s()
	{
		displayDevice = { sizeof(displayDevice), {0} };
		memoryStatus = { sizeof(memoryStatus) , {0} };
		availDiskSpace = 0;
		totalDiskSpace = 0;
	}

	DISPLAY_DEVICE displayDevice;
	MEMORYSTATUSEX memoryStatus;

	size_t totalDiskSpace;
	size_t availDiskSpace;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCrashHandler
{
public:
	CCrashHandler();
	~CCrashHandler();

	void Init();
	void Shutdown();

	void Reset();

	//-------------------------------------------------------------------------
	// Inlines: 
	//-------------------------------------------------------------------------
	void Start();
	void End();

	inline void SetExit(const bool bExit) { m_bExit = bExit; };
	inline bool GetExit() const { return m_bExit; };

	inline bool IsValid() const { return m_hExceptionHandler != nullptr; };

	inline bool CrashingModuleNameKnown() const { return m_CrashingModule.Length() > 0; }
	inline const char* GetCrashingModuleName() const { return m_CrashingModule.String(); }

	inline const EXCEPTION_POINTERS* GetExceptionPointers() const { return m_pExceptionPointers; }
	inline const CrashHardWareInfo_s& GetHardwareInfo() const { return m_HardWareInfo; }

	//-------------------------------------------------------------------------
	// Formatters: 
	//-------------------------------------------------------------------------
	void FormatCrash();
	void FormatCallstack();
	void FormatRegisters();
	void FormatModules();
	void FormatSystemInfo();
	void FormatBuildInfo();

	//-------------------------------------------------------------------------
	// Utility: 
	//-------------------------------------------------------------------------
	const char* ExceptionToString() const;
	const char* ExceptionToString(const DWORD nExceptionCode) const;

	void SetExceptionPointers(EXCEPTION_POINTERS* const pExceptionPointers) { m_pExceptionPointers = pExceptionPointers; }
	void SetCrashCallback(CrashCallback_t pCrashCallback) { m_pCrashCallback = pCrashCallback; }

	void CaptureCallStack();
	void WriteFile();

	void CreateMessageProcess() const;
	void CrashCallback() const;

private:

	//-------------------------------------------------------------------------
	// Internals: 
	//-------------------------------------------------------------------------
	void FormatExceptionAddress();
	void FormatExceptionAddress(const LPCSTR pExceptionAddress);
	void FormatExceptionCode();

	void FormatALU(const char* const pszRegister, const DWORD64 nContent);
	void FormatFPU(const char* const pszRegister, const M128A* const pxContent);

	bool IsPageAccessible() const;

private:
	enum
	{
		NUM_FRAMES_TO_CAPTURE = 128,
		MAX_MODULE_HANDLES    = 4096
	};

	// Captured callstack frames.
	PVOID m_ppStackTrace[NUM_FRAMES_TO_CAPTURE];
	WORD m_nCapturedFrames;

	// The loaded module handles that are being tracked.
	HMODULE m_ppModuleHandles[MAX_MODULE_HANDLES];

	// Custom crash callback that's called after the logs have been written.
	CrashCallback_t m_pCrashCallback;

	// Current exception handler, only kept here for tracking as we need the
	// handle if we want to remove this handler.
	PVOID m_hExceptionHandler;

	EXCEPTION_POINTERS* m_pExceptionPointers;

	// 32KiB buffer containing the entire crash log, static as we shouldn't
	// allocate any dynamic memory when writing the log files as that is
	// unsafe during a crash.
	CFmtStrQuietTruncationN<32768> m_Buffer;

	// Buffer containing the module name we crashed in.
	CFmtStrQuietTruncationN<256> m_CrashingModule;

	uint8_t m_nCrashMsgFlags;

	// Set when called to prevent recursive calls.
	bool m_bExit;

	// Set when crashmsg.exe is created to prevent recursive messages.
	bool m_bMessageCreated;
	
	// Cached hardware info this application crashed on.
	CrashHardWareInfo_s m_HardWareInfo;

	mutable RTL_SRWLOCK m_Lock;
};

extern CCrashHandler g_CrashHandler;

#endif // CRASHHANDLER_H