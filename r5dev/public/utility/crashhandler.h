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


	static const char* ExceptionToString(DWORD nExceptionCode);

	void FormatCrash();
	void FormatCallstack();
	void FormatRegisters();

	void FormatAPU(const char* pszRegister, DWORD64 nContent);
	void FormatFPU(const char* pszRegister, M128A* pxContent);


	void FormatExceptionAddress(LPCSTR pExceptionAddress = nullptr);
	void FormatExceptionCode();


	void GetCallStack();


	bool IsPageAccessible();


	void SetExceptionPointers(EXCEPTION_POINTERS* pExceptionPointers) { m_pExceptionPointers = pExceptionPointers; };


	void WriteFile();


	void Lock() const { m_Mutex.lock(); };
	void Unlock() const { m_Mutex.unlock(); };

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