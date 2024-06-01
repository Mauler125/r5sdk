//=============================================================================//
//
// Purpose: Squirrel VM
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/platform_internal.h"
#include "tier0/commandline.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // CLIENT_DLL
#ifndef DEDICATED
#include "engine/client/cdll_engine_int.h"
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#include "squirrel.h"
#include "sqvm.h"
#include "sqstate.h"
#include "sqstdaux.h"

//---------------------------------------------------------------------------------
// Console variables
//---------------------------------------------------------------------------------
static ConVar script_show_output("script_show_output", "0", FCVAR_RELEASE, "Prints the VM output to the console ( !slower! ).", true, 0.f, true, 2.f, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify");
static ConVar script_show_warning("script_show_warning", "0", FCVAR_RELEASE, "Prints the VM warning output to the console ( !slower! ).", true, 0.f, true, 2.f, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify");

//---------------------------------------------------------------------------------
// Purpose: prints the output of each VM to the console
// Input  : *sqvm - 
//			*fmt - 
//			... - 
//---------------------------------------------------------------------------------
SQRESULT SQVM_PrintFunc(HSQUIRRELVM v, SQChar* fmt, ...)
{
	eDLL_T remoteContext;
	// We use the sqvm pointer as index for SDK usage as the function prototype has to match assembly.
	// The compiler 'pointer truncation' warning couldn't be avoided, but it's safe to ignore it here.
#pragma warning(push)
#pragma warning(disable : 4302 4311)
	switch (static_cast<SQCONTEXT>(reinterpret_cast<int>(v)))
#pragma warning(pop)
	{
	case SQCONTEXT::SERVER:
		remoteContext = eDLL_T::SCRIPT_SERVER;
		break;
	case SQCONTEXT::CLIENT:
		remoteContext = eDLL_T::SCRIPT_CLIENT;
		break;
	case SQCONTEXT::UI:
		remoteContext = eDLL_T::SCRIPT_UI;
		break;
	case SQCONTEXT::NONE:
		remoteContext = eDLL_T::NONE;
		break;
	default:

		SQCONTEXT scriptContext = v->GetContext();
		switch (scriptContext)
		{
		case SQCONTEXT::SERVER:
			remoteContext = eDLL_T::SCRIPT_SERVER;
			break;
		case SQCONTEXT::CLIENT:
			remoteContext = eDLL_T::SCRIPT_CLIENT;
			break;
		case SQCONTEXT::UI:
			remoteContext = eDLL_T::SCRIPT_UI;
			break;
		default:
			remoteContext = eDLL_T::NONE;
			break;
		}
		break;
	}

	// Determine whether this is an info or warning log.
	bool bLogLevelOverride = (g_bSQAuxError || (g_bSQAuxBadLogic && v == g_pErrorVM));
	LogLevel_t level = LogLevel_t(script_show_output.GetInt());
	LogType_t type = bLogLevelOverride ? LogType_t::SQ_WARNING : LogType_t::SQ_INFO;

	// Always log script related problems to the console.
	if (type == LogType_t::SQ_WARNING &&
		level == LogLevel_t::LEVEL_DISK_ONLY)
	{
		level = LogLevel_t::LEVEL_CONSOLE;
	}

	va_list args;
	va_start(args, fmt);
	CoreMsgV(type, level, remoteContext, "squirrel_re", fmt, args);
	va_end(args);

	return SQ_OK;
}

//---------------------------------------------------------------------------------
// Purpose: sprintf from SQVM stack
// Input  : *sqvm - 
//			a2 - 
//			a3 - 
//			*nStringSize - 
//			**ppString - 
//---------------------------------------------------------------------------------
SQRESULT SQVM_sprintf(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString)
{
	static void* const retaddr = reinterpret_cast<void*>(CMemory(v_SQVM_WarningCmd).Offset(0x10).FindPatternSelf("85 ?? ?? 99", CMemory::Direction::DOWN).GetPtr());
	const SQRESULT result = v_SQVM_sprintf(v, a2, a3, nStringSize, ppString);

	if (retaddr == _ReturnAddress()) // Check if its SQVM_Warning calling.
	{
		const SQCONTEXT scriptContext = v->GetContext();
		eDLL_T remoteContext;

		switch (scriptContext)
		{
		case SQCONTEXT::SERVER:
			remoteContext = eDLL_T::SCRIPT_SERVER;
			break;
		case SQCONTEXT::CLIENT:
			remoteContext = eDLL_T::SCRIPT_CLIENT;
			break;
		case SQCONTEXT::UI:
			remoteContext = eDLL_T::SCRIPT_UI;
			break;
		default:
			remoteContext = eDLL_T::NONE;
			break;
		}

		const std::string svConstructor(*ppString, *nStringSize); // Get string from memory via std::string constructor.
		CoreMsg(LogType_t::SQ_WARNING, static_cast<LogLevel_t>(script_show_warning.GetInt()),
			remoteContext, NO_ERROR, "squirrel_re(warning)", "%s", svConstructor.c_str());
	}

	return result;
}

//---------------------------------------------------------------------------------
// Purpose: prints the compile error and context to the console
// Input  : *sqvm - 
//			*pszError - 
//			*pszFile - 
//			nLine - 
//			nColumn - 
//---------------------------------------------------------------------------------
void SQVM_CompileError(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn)
{
	static char szContextBuf[256]{};
	v_SQVM_GetErrorLine(pszFile, nLine, szContextBuf, sizeof(szContextBuf) - 1);

	const eDLL_T context = v->GetNativeContext();
	Error(context, NO_ERROR, "%s SCRIPT COMPILE ERROR: %s\n", v->GetContextName(), pszError);
	Error(context, NO_ERROR, " -> %s\n\n", szContextBuf);
	Error(context, NO_ERROR, "%s line [%d] column [%d]\n", pszFile, nLine, nColumn);
}

//---------------------------------------------------------------------------------
// Purpose: prints the logic error and context to the console
// Input  : bPrompt - 
//---------------------------------------------------------------------------------
void SQVM_LogicError(SQBool bPrompt)
{
	if ((*g_flErrorTimeStamp) > 0.0 && (bPrompt || Plat_FloatTime() > (*g_flErrorTimeStamp) + 0.0))
	{
		g_bSQAuxBadLogic = true;
	}
	else
	{
		g_bSQAuxBadLogic = false;
		g_pErrorVM = nullptr;
	}
	v_SQVM_LogicError(bPrompt);
}

void SQVM::Pop() {
	_stack[--_top] = _null_;
}

void SQVM::Pop(SQInteger n) {
	for (SQInteger i = 0; i < n; i++) {
		_stack[--_top] = _null_;
	}
}

void SQVM::Push(const SQObjectPtr& o) { _stack[_top++] = o; }
SQObjectPtr& SQVM::Top() { return _stack[_top - 1]; }
SQObjectPtr& SQVM::PopGet() { return _stack[--_top]; }
SQObjectPtr& SQVM::GetUp(SQInteger n) { return _stack[_top + n]; }
SQObjectPtr& SQVM::GetAt(SQInteger n) { return _stack[n]; }

#include "vscript/languages/squirrel_re/vsquirrel.h"
CSquirrelVM* SQVM::GetScriptVM()
{
	return _sharedstate->GetScriptVM();
}

SQChar* SQVM::GetContextName()
{
	return _sharedstate->_contextname;
}

SQCONTEXT SQVM::GetContext()
{
	return GetScriptVM()->GetContext();
}

eDLL_T SQVM::GetNativeContext()
{
	return (eDLL_T)GetContext();
}

//---------------------------------------------------------------------------------
void VSquirrelVM::Detour(const bool bAttach) const
{
	DetourSetup(&v_SQVM_PrintFunc, &SQVM_PrintFunc, bAttach);
	DetourSetup(&v_SQVM_sprintf, &SQVM_sprintf, bAttach);
	DetourSetup(&v_SQVM_CompileError, &SQVM_CompileError, bAttach);
	DetourSetup(&v_SQVM_LogicError, &SQVM_LogicError, bAttach);
}
