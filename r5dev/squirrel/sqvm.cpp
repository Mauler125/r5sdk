//=============================================================================//
//
// Purpose: Squirrel VM
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/platform_internal.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // CLIENT_DLL
#ifndef DEDICATED
#include "client/cdll_engine_int.h"
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#include "squirrel/sqtype.h"
#include "squirrel/sqvm.h"
#include "squirrel/sqinit.h"
#include "squirrel/sqstdaux.h"
#include "squirrel/sqstate.h"

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
	// The compiler 'pointer truncation' warning couldn't be avoided, but it's safe to ignore it.
	switch (static_cast<SQCONTEXT>(reinterpret_cast<int>(v)))
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

	// Always show script errors.
	bool bLogLevelOverride = (g_bSQAuxError || g_bSQAuxBadLogic && v == g_pErrorVM);
	LogType_t type = bLogLevelOverride ? LogType_t::SQ_WARNING : LogType_t::SQ_INFO;

	va_list args;
	va_start(args, fmt);
	CoreMsgV(type, static_cast<LogLevel_t>(script_show_output->GetInt()), remoteContext, "squirrel_re", fmt, args);
	va_end(args);

	return SQ_OK;
}

//---------------------------------------------------------------------------------
// Purpose: prints the warning output of each VM to the console
// Input  : *sqvm - 
//			a2 - 
//			a3 - 
//			*nStringSize - 
//			**ppString - 
//---------------------------------------------------------------------------------
SQRESULT SQVM_WarningFunc(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString)
{
	static void* retaddr = reinterpret_cast<void*>(p_SQVM_WarningCmd.Offset(0x10).FindPatternSelf("85 ?? ?? 99", CMemory::Direction::DOWN).GetPtr());
	SQRESULT result = v_SQVM_WarningFunc(v, a2, a3, nStringSize, ppString);

	if (retaddr != _ReturnAddress()) // Check if its SQVM_Warning calling.
	{
		return result;
	}

	SQCONTEXT scriptContext = v->GetContext();
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

	std::string svConstructor(*ppString, *nStringSize); // Get string from memory via std::string constructor.
	CoreMsg(LogType_t::SQ_WARNING, static_cast<LogLevel_t>(script_show_warning->GetInt()),
		remoteContext, NO_ERROR, "squirrel_re(warning)", "%s", svConstructor.c_str());

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
	static SQCONTEXT context{};
	static char szContextBuf[256]{};

	context = v->GetContext();
	v_SQVM_GetErrorLine(pszFile, nLine, szContextBuf, sizeof(szContextBuf) - 1);

	Error(static_cast<eDLL_T>(context), NO_ERROR, "%s SCRIPT COMPILE ERROR: %s\n", SQVM_GetContextName(context), pszError);
	Error(static_cast<eDLL_T>(context), NO_ERROR, " -> %s\n\n", szContextBuf);
	Error(static_cast<eDLL_T>(context), NO_ERROR, "%s line [%d] column [%d]\n", pszFile, nLine, nColumn);
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

//---------------------------------------------------------------------------------
// Purpose: Returns the VM name by context
// Input  : context - 
// Output : const SQChar* 
//---------------------------------------------------------------------------------
const SQChar* SQVM_GetContextName(SQCONTEXT context)
{
	switch (context)
	{
	case SQCONTEXT::SERVER:
		return "SERVER";
	case SQCONTEXT::CLIENT:
		return "CLIENT";
	case SQCONTEXT::UI:
		return "UI";
	default:
		return nullptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Returns the VM context by name
// Input  : *sqvm - 
// Output : const SQCONTEXT* 
//---------------------------------------------------------------------------------
const SQCONTEXT SQVM_GetContextIndex(HSQUIRRELVM v)
{
	if (strcmp(v->_sharedstate->_contextname, "SERVER") == 0)
		return SQCONTEXT::SERVER;
	if (strcmp(v->_sharedstate->_contextname, "CLIENT") == 0)
		return SQCONTEXT::CLIENT;
	if (strcmp(v->_sharedstate->_contextname, "UI") == 0)
		return SQCONTEXT::UI;

	return SQCONTEXT::NONE;
}

//---------------------------------------------------------------------------------
void HSQVM::Attach() const
{
	DetourAttach((LPVOID*)&v_SQVM_PrintFunc, &SQVM_PrintFunc);
	DetourAttach((LPVOID*)&v_SQVM_WarningFunc, &SQVM_WarningFunc);
	DetourAttach((LPVOID*)&v_SQVM_CompileError, &SQVM_CompileError);
	DetourAttach((LPVOID*)&v_SQVM_LogicError, &SQVM_LogicError);
}
//---------------------------------------------------------------------------------
void HSQVM::Detach() const
{
	DetourDetach((LPVOID*)&v_SQVM_PrintFunc, &SQVM_PrintFunc);
	DetourDetach((LPVOID*)&v_SQVM_WarningFunc, &SQVM_WarningFunc);
	DetourDetach((LPVOID*)&v_SQVM_CompileError, &SQVM_CompileError);
	DetourDetach((LPVOID*)&v_SQVM_LogicError, &SQVM_LogicError);
}
