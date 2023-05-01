#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#define CRASHHANDLER_MAX_MODULES 4096
#define CRASHMESSAGE_MSG_EXECUTABLE "bin\\crashmsg.exe"

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

	//-------------------------------------------------------------------------
	// Inlines: 
	//-------------------------------------------------------------------------
	void Start();
	void End();

	void Lock() const { m_Mutex.lock(); };
	void Unlock() const { m_Mutex.unlock(); };

	void SetState(bool bState) { m_bCallState = bState; };
	bool GetState() const { return m_bCallState; };

	bool IsValid() const { return m_hExceptionHandler != nullptr; };
	bool Handled() const { return m_bExceptionHandled; };

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
	const CHAR* ExceptionToString() const;
	const CHAR* ExceptionToString(DWORD nExceptionCode) const;
	void SetExceptionPointers(EXCEPTION_POINTERS* pExceptionPointers) { m_pExceptionPointers = pExceptionPointers; };

	void AddWhitelist(void* pWhitelist);
	void RemoveWhitelist(void* pWhitelist);
	bool HasWhitelist();

	void GetCallStack();
	void WriteFile();

	void CreateMessageProcess();

private:

	//-------------------------------------------------------------------------
	// Internals: 
	//-------------------------------------------------------------------------
	void FormatExceptionAddress();
	void FormatExceptionAddress(LPCSTR pExceptionAddress);
	void FormatExceptionCode();

	void FormatALU(const CHAR* pszRegister, DWORD64 nContent);
	void FormatFPU(const CHAR* pszRegister, M128A* pxContent);

	bool IsPageAccessible() const;

private:
	enum
	{
		NUM_FRAMES_TO_SEARCH = 7,
		NUM_FRAMES_TO_CAPTURE = 128
	};

	PVOID m_hExceptionHandler;
	PVOID m_ppStackTrace[NUM_FRAMES_TO_CAPTURE];
	EXCEPTION_POINTERS* m_pExceptionPointers;
	WORD m_nCapturedFrames;

	string m_svBuffer;       // Buffer containing the entire crash log.
	string m_svCrashMsgInfo;
	uint8_t m_nCrashMsgFlags;

	bool m_bCallState;        // Set when called to prevent recursive calls.
	bool m_bCrashMsgCreated;  // Set when crashmsg.exe is created to prevent recursive messages.
	bool m_bExceptionHandled; // Set on filter entry, unset within the same lock if exception was not handled, never unset if handled.

	std::set<void*> m_WhiteList;
	mutable std::mutex m_Mutex;
};

extern CCrashHandler* g_CrashHandler;

#endif // CRASHHANDLER_H