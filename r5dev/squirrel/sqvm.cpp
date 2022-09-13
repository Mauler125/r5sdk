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
#include "tier1/IConVar.h"
#ifdef DEDICATED
#include "engine/server/sv_rcon.h"
#else // DEDICATED
#include "client/cdll_engine_int.h"
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#endif
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
	SQCONTEXT context;
	int nResponseId;
	// We use the sqvm pointer as index for SDK usage as the function prototype has to match assembly.
	switch (static_cast<SQCONTEXT>(reinterpret_cast<int>(v)))
	{
	case SQCONTEXT::SERVER:
		context = SQCONTEXT::SERVER;
		nResponseId = -3;
		break;
	case SQCONTEXT::CLIENT:
		context = SQCONTEXT::CLIENT;
		nResponseId = -2;
		break;
	case SQCONTEXT::UI:
		context = SQCONTEXT::UI;
		nResponseId = -1;
		break;
	case SQCONTEXT::NONE:
		context = SQCONTEXT::NONE;
		nResponseId = -4;
		break;
	default:

		context = v->GetContext();
		switch (context)
		{
		case SQCONTEXT::SERVER:
			nResponseId = -3;
			break;
		case SQCONTEXT::CLIENT:
			nResponseId = -2;
			break;
		case SQCONTEXT::UI:
			nResponseId = -1;
			break;
		case SQCONTEXT::NONE:
			nResponseId = -4;
			break;
		default:
			nResponseId = -4;
			break;
		}
		break;
	}

	static SQChar buf[4096] = {};
	static std::string vmStr;
	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sqvm_info");

	s_LogMutex.lock();
	const char* pszUpTime = Plat_GetProcessUpTime();

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	vmStr = pszUpTime;
	vmStr.append(SQVM_LOG_T[static_cast<SQInteger>(context)]);
	vmStr.append(buf);

	if (sq_showvmoutput->GetInt() > 0) {
		sqlogger->debug(vmStr);
	}

	// Always show script errors.
	bool bLogLevelOverride = (g_bSQAuxError || g_bSQAuxBadLogic && v == g_pErrorVM);

	if (sq_showvmoutput->GetInt() > 1 || bLogLevelOverride)
	{
		bool bColorOverride = false;
		bool bError = false;

		if (g_bSQAuxError)
		{
			bColorOverride = true;
			if (strstr(buf, "SCRIPT ERROR:") || strstr(buf, " -> ")) {
				bError = true;
			}
		}
		if (g_bSQAuxBadLogic)
		{
			if (strstr(buf, "There was a problem processing game logic."))
			{
				bColorOverride = true;
				bError = true;
				g_bSQAuxBadLogic = false;
			}
		}

		if (!g_bSpdLog_UseAnsiClr)
		{
			wconsole->debug(vmStr);
#ifdef DEDICATED
			RCONServer()->Send(vmStr, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG, nResponseId);
#endif // DEDICATED
		}
		else // Use ANSI escape codes for the external console.
		{
			static std::string vmStrAnsi;
			vmStrAnsi = pszUpTime;
			if (bColorOverride)
			{
				if (bError) {
					vmStrAnsi.append(SQVM_ERROR_ANSI_LOG_T[static_cast<SQInteger>(context)]);
				}
				else {
					vmStrAnsi.append(SQVM_WARNING_ANSI_LOG_T[static_cast<SQInteger>(context)]);
				}
			}
			else {
				vmStrAnsi.append(SQVM_ANSI_LOG_T[static_cast<SQInteger>(context)]);
			}
			vmStrAnsi.append(buf);
			wconsole->debug(vmStrAnsi);
#ifdef DEDICATED
			RCONServer()->Send(vmStrAnsi, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG, nResponseId);
#endif // DEDICATED
		}

#ifndef DEDICATED
		vmStr = std::regex_replace(vmStr, rxAnsiExp, "");
		iconsole->debug(vmStr);

		if (sq_showvmoutput->GetInt() > 2 || bLogLevelOverride)
		{
			ImVec4 color;
			if (bColorOverride)
			{
				if (bError) {
					color = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
				}
				else {
					color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f);
				}
			}
			else
			{
				switch (context)
				{
				case SQCONTEXT::SERVER:
					color = ImVec4(0.59f, 0.58f, 0.73f, 1.00f);
					break;
				case SQCONTEXT::CLIENT:
					color = ImVec4(0.59f, 0.58f, 0.63f, 1.00f);
					break;
				case SQCONTEXT::UI:
					color = ImVec4(0.59f, 0.48f, 0.53f, 1.00f);
					break;
				default:
					color = ImVec4(0.59f, 0.58f, 0.63f, 1.00f);
					break;
				}
			}

			g_pConsole->AddLog(ConLog_t(g_spd_sys_w_oss.str(), color));
			g_pLogSystem.AddLog(static_cast<EGlobalContext_t>(nResponseId), g_spd_sys_w_oss.str());
		}
#endif // !DEDICATED
	}

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();

	s_LogMutex.unlock();
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
	int nResponseId;
	SQCONTEXT context;
	SQRESULT result = v_SQVM_WarningFunc(v, a2, a3, nStringSize, ppString);

	if (retaddr != _ReturnAddress() || !sq_showvmwarning->GetBool()) // Check if its SQVM_Warning calling.
	{
		return result;
	}

	s_LogMutex.lock();
	const char* pszUpTime = Plat_GetProcessUpTime();
#ifdef GAMEDLL_S3
	context = v->GetContext();
#else // Nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	context = SQVM_GetContextIndex(v);
#endif

	switch (context)
	{
	case SQCONTEXT::SERVER:
		nResponseId = -3;
		break;
	case SQCONTEXT::CLIENT:
		nResponseId = -2;
		break;
	case SQCONTEXT::UI:
		nResponseId = -1;
		break;
	case SQCONTEXT::NONE:
		nResponseId = -4;
		break;
	default:
		nResponseId = -4;
		break;
	}

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sqvm_warn");

	std::string vmStr = pszUpTime;
	vmStr.append(SQVM_LOG_T[static_cast<int>(context)]);
	std::string svConstructor(*ppString, *nStringSize); // Get string from memory via std::string constructor.
	vmStr.append(svConstructor);

	sqlogger->debug(vmStr); // Emit to file.
	if (sq_showvmwarning->GetInt() > 1)
	{
		if (!g_bSpdLog_UseAnsiClr)
		{
			wconsole->debug(vmStr);
#ifdef DEDICATED
			RCONServer()->Send(vmStr, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG, nResponseId);
#endif // DEDICATED
		}
		else
		{
			std::string vmStrAnsi = pszUpTime;
			vmStrAnsi.append(SQVM_WARNING_ANSI_LOG_T[static_cast<int>(context)]);
			vmStrAnsi.append(svConstructor);
			wconsole->debug(vmStrAnsi);
#ifdef DEDICATED
			RCONServer()->Send(vmStrAnsi, "", sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG, nResponseId);
#endif // DEDICATED
		}

#ifndef DEDICATED
		iconsole->debug(vmStr); // Emit to in-game console.

		g_pConsole->AddLog(ConLog_t(g_spd_sys_w_oss.str(), ImVec4(1.00f, 1.00f, 0.00f, 0.80f)));
		g_pLogSystem.AddLog(EGlobalContext_t::WARNING_C, g_spd_sys_w_oss.str());
#endif // !DEDICATED
	}

	g_spd_sys_w_oss.str("");
	g_spd_sys_w_oss.clear();

	s_LogMutex.unlock();
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

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	context = v->GetContext();
#else // Nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	context = SQVM_GetContextIndex(v);
#endif

	v_SQVM_GetErrorLine(pszFile, nLine, szContextBuf, sizeof(szContextBuf));

	Error(static_cast<eDLL_T>(context), NULL, "%s SCRIPT COMPILE ERROR: %s\n", SQVM_GetContextName(context), pszError);
	Error(static_cast<eDLL_T>(context), NULL, " -> %s\n\n", szContextBuf);
	Error(static_cast<eDLL_T>(context), NULL, "%s line [%d] column [%d]\n", pszFile, nLine, nColumn);
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
void SQVM_Attach()
{
	DetourAttach((LPVOID*)&v_SQVM_PrintFunc, &SQVM_PrintFunc);
	DetourAttach((LPVOID*)&v_SQVM_WarningFunc, &SQVM_WarningFunc);
	DetourAttach((LPVOID*)&v_SQVM_CompileError, &SQVM_CompileError);
	DetourAttach((LPVOID*)&v_SQVM_LogicError, &SQVM_LogicError);
}
//---------------------------------------------------------------------------------
void SQVM_Detach()
{
	DetourDetach((LPVOID*)&v_SQVM_PrintFunc, &SQVM_PrintFunc);
	DetourDetach((LPVOID*)&v_SQVM_WarningFunc, &SQVM_WarningFunc);
	DetourDetach((LPVOID*)&v_SQVM_CompileError, &SQVM_CompileError);
	DetourDetach((LPVOID*)&v_SQVM_LogicError, &SQVM_LogicError);
}
