//=============================================================================//
//
// Purpose: Crash handler (overrides the game's implementation!)
//
//=============================================================================//
#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/cpu.h"
#include "public/utility/binstream.h"
#include "public/utility/crashhandler.h"

//-----------------------------------------------------------------------------
// Purpose: formats the crasher (module, address and exception)
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCrash()
{
	m_svBuffer.append("crash:\n{\n");

	FormatExceptionAddress();
	FormatExceptionCode();

	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the captured callstack
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCallstack()
{
	m_svBuffer.append("callstack:\n{\n");

	if (m_nCapturedFrames)
	{
		PEXCEPTION_RECORD pExceptionRecord = m_pExceptionPointers->ExceptionRecord;
		if (m_ppStackTrace[m_nCapturedFrames - 1] == pExceptionRecord->ExceptionAddress)
		{
			PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
			MEMORY_BASIC_INFORMATION mbi = { 0 };
			SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &mbi, sizeof(LPCVOID));

			if (t >= sizeof(mbi)
				&& !(mbi.Protect & PAGE_NOACCESS)
				&& (mbi.Protect & PAGE_READONLY | PAGE_READWRITE)
				&& (mbi.State & MEM_COMMIT))
			{
				m_svBuffer.append("\t// call stack ended; possible return address?\n");
			}
		}
	}
	for (WORD i = 0; i < m_nCapturedFrames; i++)
	{
		FormatExceptionAddress(reinterpret_cast<LPCSTR>(m_ppStackTrace[i]));
	}

	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats all the registers and their contents
//-----------------------------------------------------------------------------
void CCrashHandler::FormatRegisters()
{
	m_svBuffer.append("registers:\n{\n");
	PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;

	FormatAPU("rax", pContextRecord->Rax);
	FormatAPU("rbx", pContextRecord->Rbx);
	FormatAPU("rcx", pContextRecord->Rcx);
	FormatAPU("rdx", pContextRecord->Rdx);
	FormatAPU("rsp", pContextRecord->Rsp);
	FormatAPU("rbp", pContextRecord->Rbp);
	FormatAPU("rsi", pContextRecord->Rsi);
	FormatAPU("rdi", pContextRecord->Rdi);
	FormatAPU("r8 ", pContextRecord->R8);
	FormatAPU("r9 ", pContextRecord->R9);
	FormatAPU("r10", pContextRecord->R10);
	FormatAPU("r11", pContextRecord->R11);
	FormatAPU("r12", pContextRecord->R12);
	FormatAPU("r13", pContextRecord->R13);
	FormatAPU("r14", pContextRecord->R14);
	FormatAPU("r15", pContextRecord->R15);
	FormatAPU("rip", pContextRecord->Rip);

	FormatFPU("xmm0 ", &pContextRecord->Xmm0);
	FormatFPU("xmm1 ", &pContextRecord->Xmm1);
	FormatFPU("xmm2 ", &pContextRecord->Xmm2);
	FormatFPU("xmm3 ", &pContextRecord->Xmm3);
	FormatFPU("xmm4 ", &pContextRecord->Xmm4);
	FormatFPU("xmm5 ", &pContextRecord->Xmm5);
	FormatFPU("xmm6 ", &pContextRecord->Xmm6);
	FormatFPU("xmm7 ", &pContextRecord->Xmm7);
	FormatFPU("xmm8 ", &pContextRecord->Xmm8);
	FormatFPU("xmm9 ", &pContextRecord->Xmm9);
	FormatFPU("xmm10", &pContextRecord->Xmm10);
	FormatFPU("xmm11", &pContextRecord->Xmm11);
	FormatFPU("xmm12", &pContextRecord->Xmm12);
	FormatFPU("xmm13", &pContextRecord->Xmm13);
	FormatFPU("xmm14", &pContextRecord->Xmm14);
	FormatFPU("xmm15", &pContextRecord->Xmm15);

	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats all loaded modules (verbose)
//-----------------------------------------------------------------------------
void CCrashHandler::FormatModules()
{
	m_svBuffer.append("modules:\n{\n");

	HMODULE hModule[4096];
	HANDLE hProcess = GetCurrentProcess();
	DWORD cbNeeded;

	BOOL result = K32EnumProcessModulesEx(hProcess, hModule, sizeof(hModule), &cbNeeded, LIST_MODULES_ALL);
	if (result && cbNeeded <= sizeof(hModule) && cbNeeded >> 3)
	{
		char szModuleName[512];
		char* pszModuleName;
		MODULEINFO modInfo;

		for (int i = 0, j = cbNeeded >> 3; j; i++, j--)
		{
			DWORD m = GetModuleFileNameA(hModule[i], szModuleName, sizeof(szModuleName));
			if ((m - 1) > (sizeof(szModuleName) - 2)) // Too small for buffer.
			{
				snprintf(szModuleName, sizeof(szModuleName), "module@%p", hModule[i]);
				pszModuleName = szModuleName;
			}
			else
			{
				pszModuleName = strrchr(szModuleName, '\\') + 1;
			}

			K32GetModuleInformation(hProcess, hModule[i], &modInfo, sizeof(modInfo));

			m_svBuffer.append(fmt::format("\t{:15s}: [0x{:016X}, 0x{:016X}]\n", 
				pszModuleName, reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll), 
				static_cast<uintptr_t>((reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll) + modInfo.SizeOfImage))));
		}
	}

	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the system information
//-----------------------------------------------------------------------------
void CCrashHandler::FormatSystemInfo()
{
	m_svBuffer.append("system:\n{\n");

	const CPUInformation& pi = GetCPUInformation();

	m_svBuffer.append(fmt::format("\tcpu_model = \"{:s}\"\n", pi.m_szProcessorBrand));
	m_svBuffer.append(fmt::format("\tcpu_speed = {:d} // clock cycles\n", pi.m_Speed));

	for (int i = 0; ; i++)
	{
		DISPLAY_DEVICE dd = { sizeof(dd), 0 };
		BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);
		if (!f)
		{
			break;
		}

		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) // The primary device is the only relevant device.
		{
			char szDeviceName[128];
			wcstombs(szDeviceName, dd.DeviceString, sizeof(szDeviceName));
			m_svBuffer.append(fmt::format("\tgpu_model = \"{:s}\"\n", szDeviceName));
			m_svBuffer.append(fmt::format("\tgpu_flags = 0x{:08X} // primary device\n", dd.StateFlags));
		}
	}

	MEMORYSTATUSEX statex{};
	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex))
	{
		m_svBuffer.append(fmt::format("\tram_total = [ {:d}, {:d} ] // physical/virtual (MiB)\n", (statex.ullTotalPhys / 1024) / 1024, (statex.ullTotalVirtual / 1024) / 1024));
		m_svBuffer.append(fmt::format("\tram_avail = [ {:d}, {:d} ] // physical/virtual (MiB)\n", (statex.ullAvailPhys / 1024) / 1024, (statex.ullAvailVirtual / 1024) / 1024));
	}

	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: formats the build information
//-----------------------------------------------------------------------------
void CCrashHandler::FormatBuildInfo()
{
	m_svBuffer.append(fmt::format("build_id: {:d}\n", g_SDKDll.m_pNTHeaders->FileHeader.TimeDateStamp));
}

//-----------------------------------------------------------------------------
// Purpose: formats the module, address and exception
// Input  : pExceptionAddress - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionAddress(LPCSTR pExceptionAddress)
{
	HMODULE hCrashedModule;
	if (!pExceptionAddress)
	{
		pExceptionAddress = static_cast<LPCSTR>(m_pExceptionPointers->ExceptionRecord->ExceptionAddress);
	}

	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, pExceptionAddress, &hCrashedModule))
	{
		m_svBuffer.append(fmt::format("\t!!!unknown-module!!!: 0x{:016X}\n", reinterpret_cast<uintptr_t>(pExceptionAddress)));
		m_nCrashMsgFlags = 0; // Display the "unknown DLL or EXE" message.
		return;
	}

	LPCSTR pModuleBase = reinterpret_cast<LPCSTR>(pExceptionAddress - reinterpret_cast<LPCSTR>(hCrashedModule));

	char szCrashedModuleFullName[512];
	if (GetModuleFileNameExA(GetCurrentProcess(), hCrashedModule, szCrashedModuleFullName, sizeof(szCrashedModuleFullName)) - 1 > 0x1FE)
	{
		m_svBuffer.append(fmt::format("\tmodule@{:016X}: 0x{:016X}\n", (void*)hCrashedModule, reinterpret_cast<uintptr_t>(pModuleBase)));
		m_nCrashMsgFlags = 0; // Display the "Apex crashed" message without additional information regarding the module.
		return;
	}

	// TODO: REMOVE EXT.
	const char* szCrashedModuleName = strrchr(szCrashedModuleFullName, '\\') + 1;
	m_svBuffer.append(fmt::format("\t{:15s}: 0x{:016X}\n", szCrashedModuleName, reinterpret_cast<uintptr_t>(pModuleBase)));
	m_nCrashMsgFlags = 1; // Display the "Apex crashed in <module>" message.

	if (m_svCrashMsgInfo.empty()) // Only set it once to the crashing module.
	{
		m_svCrashMsgInfo = szCrashedModuleName;
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the exception code
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionCode()
{
	DWORD nExceptionCode = m_pExceptionPointers->ExceptionRecord->ExceptionCode;
	if (nExceptionCode > EXCEPTION_IN_PAGE_ERROR)
	{
		m_svBuffer.append(fmt::format(ExceptionToString(), nExceptionCode));
	}
	else if (nExceptionCode >= EXCEPTION_ACCESS_VIOLATION)
	{
		const char* pszException = "EXCEPTION_IN_PAGE_ERROR";
		if (nExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		{
			pszException = "EXCEPTION_ACCESS_VIOLATION";
		}

		ULONG_PTR uExceptionInfo0 = m_pExceptionPointers->ExceptionRecord->ExceptionInformation[0];
		ULONG_PTR uExceptionInfo1 = m_pExceptionPointers->ExceptionRecord->ExceptionInformation[1];

		if (uExceptionInfo0)
		{
			if (uExceptionInfo0 == 1)
			{
				m_svBuffer.append(fmt::format("\t{:s}(write): 0x{:016X}\n", pszException, uExceptionInfo1));
			}
			else if (uExceptionInfo0 == 8)
			{
				m_svBuffer.append(fmt::format("\t{:s}(execute): 0x{:016X}\n", pszException, uExceptionInfo1));
			}
			else
			{
				m_svBuffer.append(fmt::format("\t{:s}(unknown): 0x{:016X}\n", pszException, uExceptionInfo1));
			}
		}
		else
		{
			m_svBuffer.append(fmt::format("\t{:s}(read): 0x{:016X}\n", pszException, uExceptionInfo1));
		}

		if (uExceptionInfo0 != 8)
		{
			if (IsPageAccessible())
			{
				FormatExceptionAddress();
			}
		}
	}
	else
	{
		m_svBuffer.append(fmt::format(ExceptionToString(), nExceptionCode));
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the register and its content
// Input  : *pszRegister - 
//			nContent - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatAPU(const char* pszRegister, DWORD64 nContent)
{
	if (abs64(nContent) >= 1000000)
	{
		if (nContent > 0xFFFFFFFF)
		{
			m_svBuffer.append(fmt::format("\t{:s} = 0x{:016X}\n", pszRegister, nContent));
		}
		else
		{
			m_svBuffer.append(fmt::format("\t{:s} = 0x{:08X}\n", pszRegister, nContent));
		}
	}
	else if (nContent < 0xFFFFFF80 || nContent > 0xFF)
	{
		m_svBuffer.append(fmt::format("\t{:s} = {:<15d} // 0x{:08X}\n", pszRegister, nContent, nContent));
	}
	else
	{
		m_svBuffer.append(fmt::format("\t{:s} = {:<10d}\n", pszRegister, nContent));
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the floating point register and its content
// Input  : *pszRegister - 
//			*pxContent - 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatFPU(const char* pszRegister, M128A* pxContent)
{
	DWORD nVec[4] =
	{
		pxContent->Low & INT_MAX,
		pxContent->Low >> 32,
		pxContent->High & INT_MAX,
		pxContent->High >> 32,
	};

	m_svBuffer.append(fmt::format("\t{:s} = [ [{:g}, {:g}, {:g}, {:g}]", pszRegister,
		*reinterpret_cast<float*>(&nVec[0]),
		*reinterpret_cast<float*>(&nVec[1]),
		*reinterpret_cast<float*>(&nVec[2]),
		*reinterpret_cast<float*>(&nVec[3])));

	const char* pszVectorFormat = ", [{:d}, {:d}, {:d}, {:d}] ]\n";
	int nHighest = *std::max_element(nVec, nVec + SDK_ARRAYSIZE(nVec));

	if (nHighest >= 1000000)
	{
		pszVectorFormat = ", [0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}] ]\n";
	}

	m_svBuffer.append(fmt::format(pszVectorFormat, nVec[0], nVec[1], nVec[2], nVec[3]));
}

//-----------------------------------------------------------------------------
// Purpose: returns the current exception code as string
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString(DWORD nExceptionCode) const
{
	switch (nExceptionCode)
	{
	case EXCEPTION_GUARD_PAGE:               { return "\tEXCEPTION_GUARD_PAGE"               ": 0x{:08X}\n"; };
	case EXCEPTION_BREAKPOINT:               { return "\tEXCEPTION_BREAKPOINT"               ": 0x{:08X}\n"; };
	case EXCEPTION_SINGLE_STEP:              { return "\tEXCEPTION_SINGLE_STEP"              ": 0x{:08X}\n"; };
	case EXCEPTION_ACCESS_VIOLATION:         { return "\tEXCEPTION_ACCESS_VIOLATION"         ": 0x{:08X}\n"; };
	case EXCEPTION_IN_PAGE_ERROR:            { return "\tEXCEPTION_IN_PAGE_ERROR"            ": 0x{:08X}\n"; };
	case EXCEPTION_INVALID_HANDLE:           { return "\tEXCEPTION_INVALID_HANDLE"           ": 0x{:08X}\n"; };
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    { return "\tEXCEPTION_ARRAY_BOUNDS_EXCEEDED"    ": 0x{:08X}\n"; };
	case EXCEPTION_ILLEGAL_INSTRUCTION:      { return "\tEXCEPTION_ILLEGAL_INSTRUCTION"      ": 0x{:08X}\n"; };
	case EXCEPTION_INVALID_DISPOSITION:      { return "\tEXCEPTION_INVALID_DISPOSITION"      ": 0x{:08X}\n"; };
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: { return "\tEXCEPTION_NONCONTINUABLE_EXCEPTION" ": 0x{:08X}\n"; };
	case EXCEPTION_PRIV_INSTRUCTION:         { return "\tEXCEPTION_PRIV_INSTRUCTION"         ": 0x{:08X}\n"; };
	case EXCEPTION_STACK_OVERFLOW:           { return "\tEXCEPTION_STACK_OVERFLOW"           ": 0x{:08X}\n"; };
	case EXCEPTION_DATATYPE_MISALIGNMENT:    { return "\tEXCEPTION_DATATYPE_MISALIGNMENT"    ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_DENORMAL_OPERAND:     { return "\tEXCEPTION_FLT_DENORMAL_OPERAND"     ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       { return "\tEXCEPTION_FLT_DIVIDE_BY_ZERO"       ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_INEXACT_RESULT:       { return "\tEXCEPTION_FLT_INEXACT_RESULT"       ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_INVALID_OPERATION:    { return "\tEXCEPTION_FLT_INVALID_OPERATION"    ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_OVERFLOW:             { return "\tEXCEPTION_FLT_OVERFLOW"             ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_STACK_CHECK:          { return "\tEXCEPTION_FLT_STACK_CHECK"          ": 0x{:08X}\n"; };
	case EXCEPTION_FLT_UNDERFLOW:            { return "\tEXCEPTION_FLT_UNDERFLOW"            ": 0x{:08X}\n"; };
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       { return "\tEXCEPTION_INT_DIVIDE_BY_ZERO"       ": 0x{:08X}\n"; };
	case EXCEPTION_INT_OVERFLOW:             { return "\tEXCEPTION_INT_OVERFLOW"             ": 0x{:08X}\n"; };
	default:                                 { return "\tUNKNOWN_EXCEPTION"                  ": 0x{:08X}\n"; };
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the current exception code as string
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString() const
{
	return ExceptionToString(m_pExceptionPointers->ExceptionRecord->ExceptionCode);
}

//-----------------------------------------------------------------------------
// Purpose: tests if memory page is accessible
//-----------------------------------------------------------------------------
bool CCrashHandler::IsPageAccessible() const
{
	PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
	MEMORY_BASIC_INFORMATION mbi = { 0 };

	SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &mbi, sizeof(LPCVOID));
	if (t < sizeof(mbi) || (mbi.Protect & PAGE_NOACCESS) || !(mbi.Protect & PAGE_NOACCESS | PAGE_READWRITE))
	{
		return false;
	}
	else
	{
		return !(mbi.State & MEM_COMMIT);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: captures the callstack
//-----------------------------------------------------------------------------
void CCrashHandler::GetCallStack()
{
	m_nCapturedFrames = RtlCaptureStackBackTrace(2, NUM_FRAMES_TO_CAPTURE, m_ppStackTrace, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: writes the formatted exception buffer to a file on the disk
//-----------------------------------------------------------------------------
void CCrashHandler::WriteFile()
{
	string svLogDirectory = fmt::format("{:s}{:s}", g_svLogSessionDirectory, "apex_crash.txt");
	CIOStream ioLogFile(svLogDirectory, CIOStream::Mode_t::WRITE);

	ioLogFile.WriteString(m_svBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: creates the crashmsg process displaying the error to the user
// the process has to be separate as the current process is getting killed
//-----------------------------------------------------------------------------
void CCrashHandler::CreateMessageProcess()
{
	if (m_bCrashMsgCreated)
	{
		return; // CrashMsg already displayed.
	}
	m_bCrashMsgCreated = true;

	PEXCEPTION_RECORD pExceptionRecord = m_pExceptionPointers->ExceptionRecord;
	PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;

	if (pExceptionRecord->ExceptionCode == 0xC0000005 &&
		pExceptionRecord->ExceptionInformation[0] == 8 &&
		pExceptionRecord->ExceptionInformation[1] != pContextRecord->Rip)
	{
		m_svCrashMsgInfo = "bin\\crashmsg.exe overclock";
	}
	else
	{
		m_svCrashMsgInfo = fmt::format("bin\\crashmsg.exe crash {:d} \"{:s}\"", m_nCrashMsgFlags, m_svCrashMsgInfo);
	}

	PROCESS_INFORMATION processInfo;
	STARTUPINFOA startupInfo = { 0 };

	startupInfo.cb = sizeof(STARTUPINFOA);

	if (CreateProcessA(NULL, (LPSTR)m_svCrashMsgInfo.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInfo))
	{
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
long __stdcall ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	g_CrashHandler->Lock();

	g_CrashHandler->SetExceptionPointers(exceptionInfo);
	if (g_CrashHandler->ExceptionToString() == g_CrashHandler->ExceptionToString(-1))
	{
		g_CrashHandler->Unlock();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (IsDebuggerPresent())
	{
		g_CrashHandler->Unlock();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Kill on recursive call.
	if (g_CrashHandler->GetState())
		ExitProcess(1u);
	g_CrashHandler->SetState(true);

	g_CrashHandler->GetCallStack();

	g_CrashHandler->FormatCrash();
	g_CrashHandler->FormatCallstack();
	g_CrashHandler->FormatRegisters();
	g_CrashHandler->FormatModules();
	g_CrashHandler->FormatSystemInfo();
	g_CrashHandler->FormatBuildInfo();

	g_CrashHandler->WriteFile();
	g_CrashHandler->CreateMessageProcess();

	g_CrashHandler->Unlock();

	return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::CCrashHandler()
	: m_ppStackTrace()
	, m_pExceptionPointers(nullptr)
	, m_nCapturedFrames(0)
	, m_nCrashMsgFlags(0)
	, m_bCallState(false)
	, m_bCrashMsgCreated(false)
{
	m_hExceptionHandler = AddVectoredExceptionHandler(TRUE, ExceptionFilter);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::~CCrashHandler()
{
	RemoveVectoredExceptionHandler(m_hExceptionHandler);
}

CCrashHandler* g_CrashHandler = new CCrashHandler();