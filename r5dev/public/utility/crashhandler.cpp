//=============================================================================//
//
// Purpose: Crash handling, it handles crashes!
//
//=============================================================================//
#include "core/stdafx.h"
#include "public/utility/binstream.h"

#ifndef _DEBUG

#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "rtech/rtech_utils.h"
#include "engine/cmodel_bsp.h"
#include "materialsystem/cmaterialglue.h"
#include "materialsystem/cmaterialsystem.h"
#include "bsplib/bsplib.h"

// Class is just for DLL init and DLL close, so we can actually define it here fine.
class CCrashHandler
{
public:
	CCrashHandler();
	~CCrashHandler();

private:
	void* m_hExceptionHandler;
};

static const std::map<DWORD, string> g_ExceptionToString = 
{
	{ EXCEPTION_ACCESS_VIOLATION,         "Access Violation" },
	{ EXCEPTION_IN_PAGE_ERROR,            "Access Violation" },
	{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED,    "Array bounds exceeded" },
	{ EXCEPTION_ILLEGAL_INSTRUCTION,      "Illegal instruction" },
	{ EXCEPTION_INVALID_DISPOSITION,      "Invalid disposition" },
	{ EXCEPTION_NONCONTINUABLE_EXCEPTION, "Non-continuable exception" },
	{ EXCEPTION_PRIV_INSTRUCTION,         "Priviledged instruction" },
	{ EXCEPTION_STACK_OVERFLOW,           "Stack overflow" },
	{ EXCEPTION_DATATYPE_MISALIGNMENT,    "Datatype misalignment" },
	{ EXCEPTION_FLT_DENORMAL_OPERAND,     "Denormal operand [FLT]" },
	{ EXCEPTION_FLT_DIVIDE_BY_ZERO,       "Divide by zero [FLT]" },
	{ EXCEPTION_FLT_INEXACT_RESULT,       "Inexact float result [FLT]" },
	{ EXCEPTION_FLT_INVALID_OPERATION,    "Invalid operation [FLT]" },
	{ EXCEPTION_FLT_OVERFLOW,             "Numeric overflow [FLT]" },
	{ EXCEPTION_FLT_STACK_CHECK,          "Stack check [FLT]" },
	{ EXCEPTION_FLT_UNDERFLOW,            "Numeric underflow [FLT]" },
	{ EXCEPTION_INT_DIVIDE_BY_ZERO,       "Divide by zero [INT]" },
	{ EXCEPTION_INT_OVERFLOW,             "Numeric overfloat [INT]" }
};

// Borrowed from the R2 project
template<> struct fmt::formatter<M128A> : fmt::formatter<string_view>
{
	template <typename FormatContext> auto format(const M128A& obj, FormatContext& ctx)
	{
		int v1 = obj.Low & INT_MAX;
		int v2 = obj.Low >> 32;
		int v3 = obj.High & INT_MAX;
		int v4 = obj.High >> 32;
		return fmt::format_to( ctx.out(),
			"[ {:G}, {:G}, {:G}, {:G}], [ 0x{:x}, 0x{:x}, 0x{:x}, 0x{:x} ]",
			*reinterpret_cast<float*>(&v1),
			*reinterpret_cast<float*>(&v2),
			*reinterpret_cast<float*>(&v3),
			*reinterpret_cast<float*>(&v4),
			v1,
			v2,
			v3,
			v4);
	}
};

string GetModuleCrashOffsetAndName(uintptr_t address)
{
	HMODULE crashedModuleHandle;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCSTR>((void*)address), &crashedModuleHandle))
		return {};

	MODULEINFO crashedModuleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), crashedModuleHandle, &crashedModuleInfo, sizeof(crashedModuleInfo)))
		return {};

	char szCrashedModuleFullName[MAX_PATH];
	if (!GetModuleFileNameExA(GetCurrentProcess(), crashedModuleHandle, szCrashedModuleFullName, MAX_PATH))
		return {};

	const char* szCrashedModuleName = strrchr(szCrashedModuleFullName, '\\') + 1;

	uintptr_t crashedModuleOffset = (address - reinterpret_cast<uintptr_t>(crashedModuleInfo.lpBaseOfDll));

	return fmt::format("{0} + 0x{1:x}", szCrashedModuleName, crashedModuleOffset);
}


string GetExceptionReason(EXCEPTION_POINTERS* exceptionInfo)
{
	const string& svException = g_ExceptionToString.at(exceptionInfo->ExceptionRecord->ExceptionCode);
	stringstream ss;
	
	ss << "Cause: " << svException << std::endl;

	switch (exceptionInfo->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
	case EXCEPTION_IN_PAGE_ERROR:
	{
		ULONG_PTR uExceptionInfo0 = exceptionInfo->ExceptionRecord->ExceptionInformation[0];
		ULONG_PTR uExceptionInfo1 = exceptionInfo->ExceptionRecord->ExceptionInformation[1];

		// I don't remember why this has been done like this, but I trust my old self for once on this.
		if (!uExceptionInfo0)
			ss << "Attempted to read from: 0x" << std::setw(8) << std::setfill('0') << std::hex << uExceptionInfo1 << std::endl;
		else if (uExceptionInfo0 == 1)
			ss << "Attempted to write to: 0x" << std::setw(8) << std::setfill('0') << std::hex << uExceptionInfo1 << std::endl;
		else if (uExceptionInfo0 == 8)
			ss << "Data Execution Prevention (DEP) at: 0x" << std::setw(8) << std::setfill('0') << std::hex << uExceptionInfo1 << std::endl;
		else
			ss << "Unknown access violation at: 0x" << std::setw(8) << std::setfill('0') << std::hex << uExceptionInfo1 << std::endl;

		return ss.str();
	}
	default:
	{
		return ss.str();
	}
	}
}

// TODO: PDB PARSE AND RUNTIME EXCP.
long __stdcall ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	// Avoid recursive calls.
	static bool bLogged = false;
	if (bLogged)
		return EXCEPTION_CONTINUE_SEARCH;

	// If you are debugging you don't need this.
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	// Goodluck on you if you somehow hit a guard page.
	if (g_ExceptionToString.find(exceptionInfo->ExceptionRecord->ExceptionCode) == g_ExceptionToString.end())
		return EXCEPTION_CONTINUE_SEARCH;

	// Now get the callstack..
	constexpr DWORD NUM_FRAMES_TO_CAPTURE = 60;
	void* pStackTrace[NUM_FRAMES_TO_CAPTURE] = { 0 };
	WORD nCapturedFrames = RtlCaptureStackBackTrace(0, NUM_FRAMES_TO_CAPTURE, pStackTrace, NULL);

#ifndef _DEBUG
	// THIS WONT WORK ON DEBUG!!!
	// THIS IS DUE TO A JMP TABLE CREATED BY MSVC!!
	static auto find_IMI_ref = CMemory(IsMaterialInternal).FindAllCallReferences(reinterpret_cast<uintptr_t>(BuildPropStaticFrustumCullMap), 1000);
	if (!find_IMI_ref.empty())
	{
		const void* imiRetAddr = find_IMI_ref.at(0).Offset(0x5).RCast<void*>();
		for (WORD i = 0; i < 10; i++)
		{
			if (imiRetAddr == pStackTrace[i])
				return EXCEPTION_CONTINUE_SEARCH;
		}
	}
#endif // _DEBUG

	CONTEXT* pExceptionContext = exceptionInfo->ContextRecord;

	// Setup message.
	string svMessage = fmt::format("{0}Module: {1}\r\n", GetExceptionReason(exceptionInfo), GetModuleCrashOffsetAndName(reinterpret_cast<uintptr_t>(exceptionInfo->ExceptionRecord->ExceptionAddress)));

	constexpr const char* stdRegs[] = { "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RIP" };
	constexpr const char* xmmRegs[] = { "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7", "XMM8", "XMM9", "XMM10", "XMM11", "XMM12", "XMM13", "XMM14", "XMM15" };

	// Normal registers now.
	for (int i = 0; i < SDK_ARRAYSIZE(stdRegs); i++)
	{
		svMessage += fmt::format("{0}: 0x{1:X}\r\n", stdRegs[i], *(DWORD64*)((std::uintptr_t)pExceptionContext + offsetof(CONTEXT, Rax) + (sizeof(DWORD64) * i)));
	}

	// FPU Time!
	for (int i = 0; i < SDK_ARRAYSIZE(xmmRegs); i++)
	{
		svMessage += fmt::format("{0}: {1}\r\n", xmmRegs[i], *(M128A*)((std::uintptr_t)pExceptionContext + offsetof(CONTEXT, Xmm0) + (sizeof(M128A) * i)));
	}

	svMessage += "\r\nStacktrace:\r\n\r\n";

	for (WORD i = 0; i < nCapturedFrames; i++)
	{
		svMessage += GetModuleCrashOffsetAndName(reinterpret_cast<uintptr_t>(pStackTrace[i])) + "\n";
	}

	std::time_t time = std::time(nullptr);
	stringstream ss; ss << "platform\\logs\\" << "apex_crash_" << std::put_time(std::localtime(&time), "%Y-%m-%d %H-%M-%S.txt");

	// Not using CBaseFileSystem here cause if that would crash we'd have a problem.
	CIOStream ioLogFile = CIOStream(ss.str(), CIOStream::Mode_t::WRITE);
	ioLogFile.WriteString(svMessage);
	ioLogFile.Close();

	MessageBoxA(0, svMessage.c_str(), "R5R Crashed :/", MB_ICONERROR | MB_OK);

	bLogged = true;

	return EXCEPTION_EXECUTE_HANDLER;
}

CCrashHandler::CCrashHandler()
{
	m_hExceptionHandler = AddVectoredExceptionHandler(TRUE, ExceptionFilter);
}

CCrashHandler::~CCrashHandler()
{
	RemoveVectoredExceptionHandler(m_hExceptionHandler);
}

// Init on DLL init!
// Needs fixing for frustum culling, it triggers the SEH handler there which also triggers this, add address to whitelist/whitelist whole code page.
CCrashHandler* g_CrashHandler = new CCrashHandler();

#endif // _DEBUG