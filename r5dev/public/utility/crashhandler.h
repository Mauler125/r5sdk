#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCrashHandler
{
public:
	CCrashHandler();
	~CCrashHandler();

	//-------------------------------------------------------------------------
	// Inlines: 
	//-------------------------------------------------------------------------
	void Lock() const { m_Mutex.lock(); };
	void Unlock() const { m_Mutex.unlock(); };
	bool GetState() const { return m_bCallState; };
	void SetState(bool bState) { m_bCallState = bState; };

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

	void FormatAPU(const CHAR* pszRegister, DWORD64 nContent);
	void FormatFPU(const CHAR* pszRegister, M128A* pxContent);

	bool IsPageAccessible() const;

private:
	enum
	{
		MAX_IMI_SEARCH = 7,
		NUM_FRAMES_TO_CAPTURE = 128
	};

	PVOID m_hExceptionHandler;
	PVOID m_ppStackTrace[NUM_FRAMES_TO_CAPTURE];
	EXCEPTION_POINTERS* m_pExceptionPointers;
	WORD m_nCapturedFrames;

	string m_svBuffer;       // Buffer containing the entire crash log.
	string m_svCrashMsgInfo;
	uint8_t m_nCrashMsgFlags;

	bool m_bCallState;       // Set when called to prevent recursive calls.
	bool m_bCrashMsgCreated; // Set when crashmsg.exe is created to prevent recursive messages.

	std::set<void*> m_WhiteList;
	mutable std::mutex m_Mutex;
};

extern CCrashHandler* g_CrashHandler;

#endif // CRASHHANDLER_H