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

	void Lock() const { m_Mutex.lock(); };
	void Unlock() const { m_Mutex.unlock(); };

	void FormatCrash();
	void FormatCallstack();
	void FormatRegisters();
	void FormatSystemInfo();
	void FormatBuildInfo();

	const char* ExceptionToString() const;

	void SetExceptionPointers(EXCEPTION_POINTERS* pExceptionPointers) { m_pExceptionPointers = pExceptionPointers; };

	void WriteFile();
	void GetCallStack();

private:

	void FormatExceptionAddress(LPCSTR pExceptionAddress = nullptr);
	void FormatExceptionCode();

	void FormatAPU(const char* pszRegister, DWORD64 nContent);
	void FormatFPU(const char* pszRegister, M128A* pxContent);

	bool IsPageAccessible() const;

private:
	enum
	{
		NUM_FRAMES_TO_CAPTURE = 60
	};

	void* m_hExceptionHandler;
	PVOID m_ppStackTrace[NUM_FRAMES_TO_CAPTURE];
	EXCEPTION_POINTERS* m_pExceptionPointers;
	WORD m_nCapturedFrames;
	string m_svBuffer;
	mutable std::mutex m_Mutex;
};

#endif // CRASHHANDLER_H