//=============================================================================//
//
// Purpose: Squirrel VM
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#include "engine/sys_utils.h"
#ifdef DEDICATED
#include "engine/sv_rcon.h"
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
	static SQCONTEXT context{};
	// We use the sqvm pointer as index for SDK usage as the function prototype has to match assembly.
	switch (static_cast<SQCONTEXT>(reinterpret_cast<int>(v)))
	{
	case SQCONTEXT::SERVER:
		context = SQCONTEXT::SERVER;
		break;
	case SQCONTEXT::CLIENT:
		context = SQCONTEXT::CLIENT;
		break;
	case SQCONTEXT::UI:
		context = SQCONTEXT::UI;
		break;
	case SQCONTEXT::NONE:
		context = SQCONTEXT::NONE;
		break;
	default:
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
		context = *reinterpret_cast<SQCONTEXT*>(reinterpret_cast<std::uintptr_t>(v) + 0x18);
#else // Nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
		context = SQVM_GetContextIndex(v);
#endif
		break;
	}
	static SQChar buf[1024] = {};
	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sqvm_info");

	s_LogMutex.lock();
	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	std::string vmStr = SQVM_LOG_T[static_cast<SQInteger>(context)].c_str();
	vmStr.append(buf);

	if (sq_showvmoutput->GetInt() > 0) {
		sqlogger->debug(vmStr);
	}
	if (sq_showvmoutput->GetInt() > 1)
	{
		bool bError = false;
		bool bColorOverride = false;
		if (!g_bSpdLog_UseAnsiClr)
		{
			wconsole->debug(vmStr);
#ifdef DEDICATED
			g_pRConServer->Send(vmStr.c_str());
#endif // DEDICATED
		}
		else
		{
			std::string vmStrAnsi;
			if (g_bSQAuxError)
			{
				bColorOverride = true;
				if (strstr(buf, "SCRIPT ERROR:") || strstr(buf, " -> "))
				{
					bError = true;
					vmStrAnsi = SQVM_ERROR_ANSI_LOG_T[static_cast<SQInteger>(context)].c_str();
				}
				else {
					vmStrAnsi = SQVM_WARNING_ANSI_LOG_T[static_cast<SQInteger>(context)].c_str();
				}
			}
			else if (g_bSQAuxBadLogic)
			{
				if (strstr(buf, "There was a problem processing game logic."))
				{
					bError = true;
					bColorOverride = true;
					g_bSQAuxBadLogic = false;
					vmStrAnsi = SQVM_ERROR_ANSI_LOG_T[static_cast<SQInteger>(context)].c_str();
				}
				else {
					vmStrAnsi = SQVM_ANSI_LOG_T[static_cast<SQInteger>(context)].c_str();
				}
			}
			else {
				vmStrAnsi = SQVM_ANSI_LOG_T[static_cast<SQInteger>(context)].c_str();
			}
			vmStrAnsi.append(buf);
			wconsole->debug(vmStrAnsi);
#ifdef DEDICATED
			g_pRConServer->Send(vmStrAnsi.c_str());
#endif // DEDICATED
		}

#ifndef DEDICATED
		vmStr = std::regex_replace(vmStr, rxAnsiExp, "");
		iconsole->debug(vmStr);

		if (sq_showvmoutput->GetInt() > 2)
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

			g_pIConsole->m_ivConLog.push_back(CConLog(PrintPercentageEscape(g_spd_sys_w_oss.str()), color));
			g_pLogSystem.AddLog(static_cast<LogType_t>(context), g_spd_sys_w_oss.str());

			g_spd_sys_w_oss.str("");
			g_spd_sys_w_oss.clear();
		}
#endif // !DEDICATED
	}

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
	static SQCONTEXT context{};
	SQRESULT result = v_SQVM_WarningFunc(v, a2, a3, nStringSize, ppString);

	if (retaddr != _ReturnAddress() || !sq_showvmwarning->GetBool()) // Check if its SQVM_Warning calling.
	{
		return result;
	}

	s_LogMutex.lock();
#ifdef GAMEDLL_S3
	context = *reinterpret_cast<SQCONTEXT*>(reinterpret_cast<std::uintptr_t>(v) + 0x18);
#else // Nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	context = SQVM_GetContextIndex(v);
#endif

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sqvm_warn");

	std::string vmStr = SQVM_LOG_T[static_cast<int>(context)].c_str();
	std::string svConstructor(*ppString, *nStringSize); // Get string from memory via std::string constructor.
	vmStr.append(svConstructor);

	sqlogger->debug(vmStr); // Emit to file.
	if (sq_showvmwarning->GetInt() > 1)
	{
		if (!g_bSpdLog_UseAnsiClr)
		{
			wconsole->debug(vmStr);
#ifdef DEDICATED
			g_pRConServer->Send(vmStr.c_str());
#endif // DEDICATED
		}
		else
		{
			std::string vmStrAnsi = SQVM_WARNING_ANSI_LOG_T[static_cast<int>(context)].c_str();
			vmStrAnsi.append(svConstructor);
			wconsole->debug(vmStrAnsi);
#ifdef DEDICATED
			g_pRConServer->Send(vmStrAnsi.c_str());
#endif // DEDICATED
		}

#ifndef DEDICATED
		iconsole->debug(vmStr); // Emit to in-game console.

		g_pIConsole->m_ivConLog.push_back(CConLog(PrintPercentageEscape(g_spd_sys_w_oss.str()), ImVec4(1.00f, 1.00f, 0.00f, 0.80f)));
		g_pLogSystem.AddLog(LogType_t::WARNING_C, g_spd_sys_w_oss.str());

		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();
#endif // !DEDICATED
	}

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
	context = *reinterpret_cast<SQCONTEXT*>(reinterpret_cast<std::uintptr_t>(v) + 0x18);
#else // Nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	context = SQVM_GetContextIndex(v);
#endif

	v_SQVM_GetErrorLine(pszFile, nLine, szContextBuf, sizeof(szContextBuf));

	Error(static_cast<eDLL_T>(context), "%s SCRIPT COMPILE ERROR: %s\n", SQVM_GetContextName(context), pszError);
	Error(static_cast<eDLL_T>(context), " -> %s\n\n", szContextBuf);
	Error(static_cast<eDLL_T>(context), "%s line [%d] column [%d]\n", pszFile, nLine, nColumn);
}

//---------------------------------------------------------------------------------
// Purpose: prints the global include file the compiler loads for loading scripts
// Input  : *szRsonName - 
//---------------------------------------------------------------------------------
SQInteger SQVM_LoadRson(const SQChar* szRsonName)
{
	if (sq_showrsonloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "\n");
		DevMsg(eDLL_T::ENGINE, "______________________________________________________________\n");
		DevMsg(eDLL_T::ENGINE, "] RSON_SQVM --------------------------------------------------\n");
		DevMsg(eDLL_T::ENGINE, "] PATH: '%s'\n", szRsonName);
		DevMsg(eDLL_T::ENGINE, "--------------------------------------------------------------\n");
		DevMsg(eDLL_T::ENGINE, "\n");
	}
	return v_SQVM_LoadRson(szRsonName);
}

//---------------------------------------------------------------------------------
// Purpose: prints the scripts the compiler loads from global include to be compiled
// Input  : *sqvm - 
//			*szScriptPath - 
//			*szScriptName - 
//			nFlag - 
//---------------------------------------------------------------------------------
SQBool SQVM_LoadScript(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag)
{
	if (sq_showscriptloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "Loading SQVM Script '%s'\n", szScriptName);
	}

	///////////////////////////////////////////////////////////////////////////////
	return v_SQVM_LoadScript(v, szScriptPath, szScriptName, nFlag);
}

//---------------------------------------------------------------------------------
// Purpose: registers and exposes code functions to target context
// Input  : *sqvm - 
//			*szName - 
//			*szHelpString - 
//			*szRetValType - 
//			*szArgTypes - 
//			*pFunction - 
//---------------------------------------------------------------------------------
SQRESULT SQVM_RegisterFunction(HSQUIRRELVM v, const SQChar* szName, const SQChar* szHelpString, const SQChar* szRetValType, const SQChar* szArgTypes, void* pFunction)
{
	SQFuncRegistration* sqFunc = new SQFuncRegistration();

	sqFunc->m_szScriptName = szName;
	sqFunc->m_szNativeName = szName;
	sqFunc->m_szHelpString = szHelpString;
	sqFunc->m_szRetValType = szRetValType;
	sqFunc->m_szArgTypes   = szArgTypes;
	sqFunc->m_pFunction    = pFunction;

	return v_SQVM_RegisterFunc(v, sqFunc, 1);
}

#ifndef CLIENT_DLL
//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void SQVM_RegisterServerScriptFunctions(HSQUIRRELVM v)
{
	SQVM_RegisterFunction(v, "SDKNativeTest", "Native SERVER test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	SQVM_RegisterFunction(v, "GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);
	SQVM_RegisterFunction(v, "GetNumHumanPlayers", "Gets the number of human players on the server", "int", "", &VSquirrel::SERVER::GetNumHumanPlayers);
	SQVM_RegisterFunction(v, "GetNumFakeClients", "Gets the number of bot players on the server", "int", "", &VSquirrel::SERVER::GetNumFakeClients);
}
#endif // !CLIENT_DLL

#ifndef DEDICATED
//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void SQVM_RegisterClientScriptFunctions(HSQUIRRELVM v)
{
	SQVM_RegisterFunction(v, "SDKNativeTest", "Native CLIENT test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	SQVM_RegisterFunction(v, "GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void SQVM_RegisterUIScriptFunctions(HSQUIRRELVM v)
{
	SQVM_RegisterFunction(v, "SDKNativeTest", "Native UI test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);

	// Functions for retrieving server browser data
	SQVM_RegisterFunction(v, "GetServerName", "Gets the name of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerName);
	SQVM_RegisterFunction(v, "GetServerPlaylist", "Gets the playlist of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerPlaylist);
	SQVM_RegisterFunction(v, "GetServerMap", "Gets the map of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerMap);
	SQVM_RegisterFunction(v, "GetServerCount", "Gets the number of public servers", "int", "", &VSquirrel::UI::GetServerCount);

	// Misc main menu functions
	SQVM_RegisterFunction(v, "GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);
	SQVM_RegisterFunction(v, "GetPromoData", "Gets promo data for specified slot type", "string", "int", &VSquirrel::UI::GetPromoData);

	// Functions for connecting to servers
	SQVM_RegisterFunction(v, "CreateServer", "Start server with the specified settings", "void", "string,string,string,int", &VSquirrel::UI::CreateServerFromMenu);
	SQVM_RegisterFunction(v, "SetEncKeyAndConnect", "Set the encryption key to that of the specified server and connects to it", "void", "int", &VSquirrel::UI::SetEncKeyAndConnect);
	SQVM_RegisterFunction(v, "JoinPrivateServerFromMenu", "Joins private server by token", "void", "string", &VSquirrel::UI::JoinPrivateServerFromMenu);
	SQVM_RegisterFunction(v, "GetPrivateServerMessage", "Gets private server join status message", "string", "string", &VSquirrel::UI::GetPrivateServerMessage);
	SQVM_RegisterFunction(v, "ConnectToIPFromMenu", "Joins server by ip and encryption key", "void", "string,string", &VSquirrel::UI::ConnectToIPFromMenu);

	SQVM_RegisterFunction(v, "GetAvailableMaps", "Gets an array of all the available maps that can be used to host a server", "array<string>", "", &VSquirrel::UI::GetAvailableMaps);
}

//---------------------------------------------------------------------------------
// Purpose: Initialize all CLIENT/UI global structs and register SDK (CLIENT/UI) script functions
// Input  : *sqvm - 
//			context - (1 = CLIENT 2 = UI)
//---------------------------------------------------------------------------------
SQInteger SQVM_InitializeCLGlobalScriptStructs(HSQUIRRELVM v, SQCONTEXT context)
{
	int results = v_SQVM_InitializeCLGlobalScriptStructs(v, context);
	if (context == SQCONTEXT::CLIENT)
		SQVM_RegisterClientScriptFunctions(g_pClientVM.GetValue<HSQUIRRELVM>());
	if (context == SQCONTEXT::UI)
		SQVM_RegisterUIScriptFunctions(g_pUIVM.GetValue<HSQUIRRELVM>());
	return results;
}
#endif // !DEDICATED

#ifndef CLIENT_DLL
//---------------------------------------------------------------------------------
// Purpose: Initialize all SERVER global structs and register SDK (SERVER) script functions
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void SQVM_InitializeSVGlobalScriptStructs(HSQUIRRELVM v)
{
	v_SQVM_InitializeSVGlobalScriptStructs(v);
	SQVM_RegisterServerScriptFunctions(g_pServerVM.GetValue<HSQUIRRELVM>());
}

//---------------------------------------------------------------------------------
// Purpose: Creates the SERVER Squirrel VM
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool SQVM_CreateServerVM()
{
	bool results = v_SQVM_CreateServerVM();
	if (results)
		DevMsg(eDLL_T::SERVER, "Created SERVER VM: '%p'\n", g_pServerVM.GetValue<HSQUIRRELVM>());
	else
		Error(eDLL_T::SERVER, "Failed to create SERVER VM\n");
	return results;
}
#endif // !CLIENT_DLL

#ifndef DEDICATED
//---------------------------------------------------------------------------------
// Purpose: Creates the CLIENT Squirrel VM
// Input  : *chlclient - 
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool SQVM_CreateClientVM(CHLClient* hlclient)
{
	bool results = v_SQVM_CreateClientVM(hlclient);
	if (results)
		DevMsg(eDLL_T::CLIENT, "Created CLIENT VM: '%p'\n", g_pClientVM.GetValue<HSQUIRRELVM>());
	else
		Error(eDLL_T::CLIENT, "Failed to create CLIENT VM\n");
	return results;
}

//---------------------------------------------------------------------------------
// Purpose: Creates the UI Squirrel VM
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool SQVM_CreateUIVM()
{
	bool results = v_SQVM_CreateUIVM();
	if (results)
		DevMsg(eDLL_T::UI, "Created UI VM: '%p'\n", g_pUIVM.GetValue<HSQUIRRELVM>());
	else
		Error(eDLL_T::UI, "Failed to create UI VM\n");
	return results;
}
#endif // !DEDICATED

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
// Purpose: Returns the VM pointer by context
// Input  : context - 
// Output : SQVM* 
//---------------------------------------------------------------------------------
HSQUIRRELVM SQVM_GetVM(SQCONTEXT context)
{
	switch (context)
	{
#ifndef CLIENT_DLL
	case SQCONTEXT::SERVER:
		return g_pServerVM.GetValue<HSQUIRRELVM>();
#endif // !CLIENT_DLL
#ifndef DEDICATED
	case SQCONTEXT::CLIENT:
		return g_pClientVM.GetValue<HSQUIRRELVM>();
	case SQCONTEXT::UI:
		return g_pUIVM.GetValue<HSQUIRRELVM>();
#endif // !DEDICATED
	default:
		return nullptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Compiles and executes input code on target VM by context
// Input  : *code - 
//			context - 
//---------------------------------------------------------------------------------
void SQVM_Execute(const SQChar* code, SQCONTEXT context)
{
	HSQUIRRELVM v = SQVM_GetVM(context);
	if (!v)
	{
		Error(eDLL_T::ENGINE, "Attempted to run %s script while VM isn't initialized\n", SQVM_GetContextName(context));
		return;
	}

	SQVM* vTable = v->GetVTable();
	SQRESULT compileResult{};
	SQBufState bufState = SQBufState(code);

	compileResult = sq_compilebuffer(vTable, &bufState, "console", -1);
	if (compileResult >= 0)
	{
		sq_pushroottable(vTable);
		SQRESULT callResult = sq_call(vTable, 1, false, false);
	}
}

//---------------------------------------------------------------------------------
void SQVM_Attach()
{
	DetourAttach((LPVOID*)&v_SQVM_PrintFunc, &SQVM_PrintFunc);
	DetourAttach((LPVOID*)&v_SQVM_WarningFunc, &SQVM_WarningFunc);
	DetourAttach((LPVOID*)&v_SQVM_CompileError, &SQVM_CompileError);
	DetourAttach((LPVOID*)&v_SQVM_LoadRson, &SQVM_LoadRson);
	DetourAttach((LPVOID*)&v_SQVM_LoadScript, &SQVM_LoadScript);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_SQVM_InitializeCLGlobalScriptStructs, &SQVM_InitializeCLGlobalScriptStructs);
#endif // !DEDICATED
#ifndef CLIENT_DLL
	DetourAttach((LPVOID*)&v_SQVM_InitializeSVGlobalScriptStructs, &SQVM_InitializeSVGlobalScriptStructs);
	DetourAttach((LPVOID*)&v_SQVM_CreateServerVM, &SQVM_CreateServerVM);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_SQVM_CreateClientVM, &SQVM_CreateClientVM);
	DetourAttach((LPVOID*)&v_SQVM_CreateUIVM, &SQVM_CreateUIVM);
#endif // !DEDICATED
}
//---------------------------------------------------------------------------------
void SQVM_Detach()
{
	DetourDetach((LPVOID*)&v_SQVM_PrintFunc, &SQVM_PrintFunc);
	DetourDetach((LPVOID*)&v_SQVM_WarningFunc, &SQVM_WarningFunc);
	DetourDetach((LPVOID*)&v_SQVM_CompileError, &SQVM_CompileError);
	DetourDetach((LPVOID*)&v_SQVM_LoadRson, &SQVM_LoadRson);
	DetourDetach((LPVOID*)&v_SQVM_LoadScript, &SQVM_LoadScript);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_SQVM_InitializeCLGlobalScriptStructs, &SQVM_InitializeCLGlobalScriptStructs);
#endif // !DEDICATED
#ifndef CLIENT_DLL
	DetourDetach((LPVOID*)&v_SQVM_InitializeSVGlobalScriptStructs, &SQVM_InitializeSVGlobalScriptStructs);
	DetourDetach((LPVOID*)&v_SQVM_CreateServerVM, &SQVM_CreateServerVM);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_SQVM_CreateClientVM, &SQVM_CreateClientVM);
	DetourDetach((LPVOID*)&v_SQVM_CreateUIVM, &SQVM_CreateUIVM);
#endif // !DEDICATED
}
