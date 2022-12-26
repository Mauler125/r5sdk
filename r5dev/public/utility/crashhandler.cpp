//=============================================================================//
//
// Purpose: Crash handler (overrides the game's implementation!)
//
//=============================================================================//
#include "core/stdafx.h"
#include "public/utility/binstream.h"

#ifndef _DEBUG

#include "crashhandler.h"
#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "rtech/rtech_utils.h"
#include "engine/cmodel_bsp.h"
#include "materialsystem/cmaterialglue.h"
#include "materialsystem/cmaterialsystem.h"
#include "bsplib/bsplib.h"

CCrashHandler* g_CrashHandler = new CCrashHandler();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CCrashHandler::ExceptionToString(DWORD nExceptionCode)
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
// Purpose: 
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
		return;
	}

	LPCSTR pModuleBase = reinterpret_cast<LPCSTR>(pExceptionAddress - reinterpret_cast<LPCSTR>(hCrashedModule));

	char szCrashedModuleFullName[512];
	if (GetModuleFileNameExA(GetCurrentProcess(), hCrashedModule, szCrashedModuleFullName, sizeof(szCrashedModuleFullName)) - 1 > 0x1FE)
	{
		m_svBuffer.append(fmt::format("\tmodule@{:016X}: 0x{:016X}\n", (void*)hCrashedModule, reinterpret_cast<uintptr_t>(pModuleBase)));
		return;
	}

	// TODO: REMOVE EXT.
	const char* szCrashedModuleName = strrchr(szCrashedModuleFullName, '\\') + 1;
	m_svBuffer.append(fmt::format("\t{:s}: 0x{:016X}\n", szCrashedModuleName, reinterpret_cast<uintptr_t>(pModuleBase)));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCrashHandler::IsPageAccessible()
{
	PCONTEXT pContextRecord = m_pExceptionPointers->ContextRecord;
	MEMORY_BASIC_INFORMATION pMemory = { 0 };

	SIZE_T t = VirtualQuery((LPCVOID)pContextRecord->Rsp, &pMemory, sizeof(LPCVOID));
	if (t < sizeof(pMemory) || (pMemory.Protect & PAGE_NOACCESS) || !(pMemory.Protect & PAGE_NOACCESS | PAGE_READWRITE))
	{
		return false;
	}
	else
	{
		return !(pMemory.State & MEM_COMMIT);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatExceptionCode()
{
	DWORD nExceptionCode = m_pExceptionPointers->ExceptionRecord->ExceptionCode;
	if (nExceptionCode > EXCEPTION_IN_PAGE_ERROR)
	{
		m_svBuffer.append(fmt::format(ExceptionToString(nExceptionCode), nExceptionCode));
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
		m_svBuffer.append(fmt::format(ExceptionToString(nExceptionCode), nExceptionCode));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::GetCallStack()
{
	m_nCapturedFrames = RtlCaptureStackBackTrace(0, NUM_FRAMES_TO_CAPTURE, m_ppStackTrace, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::WriteFile()
{
	std::time_t time = std::time(nullptr);
	stringstream ss; ss << "platform\\logs\\" << "apex_crash_" << std::put_time(std::localtime(&time), "%Y-%m-%d %H-%M-%S.txt");

	CIOStream ioLogFile = CIOStream(ss.str(), CIOStream::Mode_t::WRITE);
	ioLogFile.WriteString(m_svBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCrash()
{
	m_svBuffer.append("crash:\n{\n");

	FormatExceptionAddress();
	FormatExceptionCode();

	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCallstack()
{
	m_svBuffer.append("callstack:\n{\n");
	for (WORD i = 0; i < m_nCapturedFrames; i++)
	{
		FormatExceptionAddress(reinterpret_cast<LPCSTR>(m_ppStackTrace[i]));
	}
	m_svBuffer.append("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
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
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatFPU(const char* pszRegister, M128A* pxContent)
{
	int nVec[4] =
	{
		pxContent->Low& INT_MAX,
		pxContent->Low >> 32,
		pxContent->High& INT_MAX,
		pxContent->High >> 32,
	};

	m_svBuffer.append(fmt::format("\t{:s} = [ [{:g}, {:g}, {:g}, {:g}]", pszRegister,
		*reinterpret_cast<float*>(&nVec[0]),
		*reinterpret_cast<float*>(&nVec[1]),
		*reinterpret_cast<float*>(&nVec[2]),
		*reinterpret_cast<float*>(&nVec[3])));

	const char* pszVectorFormat = ", [{:d}, {:d}, {:d}, {:d}] ]\n";
	int nHighest = *std::max_element(nVec, nVec +4);

	if (nHighest >= 1000000)
	{
		pszVectorFormat = ", [0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}] ]\n";
	}

	m_svBuffer.append(fmt::format(pszVectorFormat, nVec[0], nVec[1], nVec[2], nVec[3]));
}

//-----------------------------------------------------------------------------
// Purpose: 
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
// Purpose: 
//-----------------------------------------------------------------------------
long __stdcall ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	g_CrashHandler->Lock();

	// Kill on recursive call.
	static bool bLogged = false;
	if (bLogged)
		ExitProcess(1u);
	bLogged = true;

	g_CrashHandler->GetCallStack();
	g_CrashHandler->SetExceptionPointers(exceptionInfo);


	g_CrashHandler->FormatCrash();
	g_CrashHandler->FormatCallstack();
	g_CrashHandler->FormatRegisters();



	g_CrashHandler->WriteFile();
	g_CrashHandler->Unlock();


//#ifndef _DEBUG
//	// THIS WONT WORK ON DEBUG!!!
//	// THIS IS DUE TO A JMP TABLE CREATED BY MSVC!!
//	static auto find_IMI_ref = CMemory(IsMaterialInternal).FindAllCallReferences(reinterpret_cast<uintptr_t>(BuildPropStaticFrustumCullMap), 1000);
//	if (!find_IMI_ref.empty())
//	{
//		const void* imiRetAddr = find_IMI_ref.at(0).Offset(0x5).RCast<void*>();
//		for (WORD i = 0; i < 7; i++)
//		{
//			if (imiRetAddr == pStackTrace[i])
//				return EXCEPTION_CONTINUE_SEARCH;
//		}
//	}
//#endif // _DEBUG

	return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrashHandler::CCrashHandler()
	: m_ppStackTrace()
	, m_nCapturedFrames(0)

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

#endif // _DEBUG