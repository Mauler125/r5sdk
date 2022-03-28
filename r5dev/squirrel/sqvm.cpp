//=============================================================================//
//
// Purpose: Squirrel VM
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "tier0/commandline.h"
#include "engine/sys_utils.h"
#ifdef DEDICATED
#include "engine/sv_rcon.h"
#endif // DEDICATED
#include "vgui/vgui_debugpanel.h"
#include "gameui/IConsole.h"
#include "squirrel/sqvm.h"
#include "squirrel/sqinit.h"

//---------------------------------------------------------------------------------
// Purpose: prints the output of each VM to the console
// Input  : *sqvm - 
//			*fmt - 
//			... - 
//---------------------------------------------------------------------------------
void* HSQVM_PrintFunc(void* sqvm, char* fmt, ...)
{
	static int vmIdx{};
	// We use the sqvm pointer as index for SDK usage as the function prototype has to match assembly.
	switch (reinterpret_cast<int>(sqvm))
	{
	case 0:
		vmIdx = 0;
		break;
	case 1:
		vmIdx = 1;
		break;
	case 2:
		vmIdx = 2;
		break;
	case 3:
		vmIdx = 3;
		break;
	default:
#ifdef GAMEDLL_S3
		vmIdx = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(sqvm) + 0x18);
#else // TODO [ AMOS ]: nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
		vmIdx = 3;
#endif
		break;
	}
	static char buf[1024] = {};
	static std::regex rxAnsiExp("\\\033\\[.*?m");

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sqvm_print_logger");

	{/////////////////////////////
		va_list args{};
		va_start(args, fmt);

		vsnprintf(buf, sizeof(buf), fmt, args);

		buf[sizeof(buf) - 1] = 0;
		va_end(args);
	}/////////////////////////////

	std::string vmStr = SQVM_LOG_T[vmIdx].c_str();
	vmStr.append(buf);

	if (sq_showvmoutput->GetInt() > 0)
	{
		sqlogger->debug(vmStr);
	}
	if (sq_showvmoutput->GetInt() > 1)
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
			std::string vmStrAnsi = SQVM_ANSI_LOG_T[vmIdx].c_str();
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
			std::string s = g_spd_sys_w_oss.str();

			g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));
			g_pLogSystem.AddLog(static_cast<LogType_t>(vmIdx), s);

			g_spd_sys_w_oss.str("");
			g_spd_sys_w_oss.clear();
		}
#endif // !DEDICATED
	}
	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: prints the warning output of each VM to the console
// Input  : *sqvm - 
//			a2 - 
//			a3 - 
//			*nStringSize - 
//			**ppString - 
//---------------------------------------------------------------------------------
void* HSQVM_WarningFunc(void* sqvm, int a2, int a3, int* nStringSize, void** ppString)
{
	static void* retaddr = reinterpret_cast<void*>(p_SQVM_WarningCmd.Offset(0x10).FindPatternSelf("85 ?? ?? 99", ADDRESS::Direction::DOWN).GetPtr());
	void* result = SQVM_WarningFunc(sqvm, a2, a3, nStringSize, ppString);

	if (retaddr != _ReturnAddress()) // Check if its SQVM_Warning calling.
	{
		return result; // If not return.
	}

#ifdef GAMEDLL_S3
	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18);
#else // TODO [ AMOS ]: nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	int vmIdx = 3;
#endif

	static std::shared_ptr<spdlog::logger> iconsole = spdlog::get("game_console");
	static std::shared_ptr<spdlog::logger> wconsole = spdlog::get("win_console");
	static std::shared_ptr<spdlog::logger> sqlogger = spdlog::get("sqvm_warn_logger");

	std::string vmStr = SQVM_WARNING_LOG_T[vmIdx].c_str();
	std::string svConstructor((char*)*ppString, *nStringSize); // Get string from memory via std::string constructor.
	vmStr.append(svConstructor);

	if (sq_showvmwarning->GetInt() > 0)
	{
		sqlogger->debug(vmStr); // Emit to file.
	}
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
			std::string vmStrAnsi = SQVM_WARNING_ANSI_LOG_T[vmIdx].c_str();
			vmStrAnsi.append(svConstructor);
			wconsole->debug(vmStrAnsi);
#ifdef DEDICATED
			g_pRConServer->Send(vmStrAnsi.c_str());
#endif // DEDICATED
		}

#ifndef DEDICATED
		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();

		iconsole->debug(vmStr); // Emit to in-game console.

		std::string s = g_spd_sys_w_oss.str();
		g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));

		if (sq_showvmwarning->GetInt() > 2)
		{
			g_pLogSystem.AddLog(LogType_t::WARNING_C, s);
			g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));
		}
#endif // !DEDICATED
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
void HSQVM_ErrorFunc(void* sqvm, const char* pszError, const char* pszFile, unsigned int nLine, int nColumn)
{
	static int vmIdx{};
	static char szContextBuf[256]{};

#ifdef GAMEDLL_S3
	vmIdx = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(sqvm) + 0x18);
#else // TODO [ AMOS ]: nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	vmIdx = 3;
#endif

	SQVM_GetErrorLine(pszFile, nLine, szContextBuf, sizeof(szContextBuf));

	Error(eDLL_T::SERVER, "%s SCRIPT COMPILE ERROR: %s\n", SQVM_TYPE_T[vmIdx].c_str(), pszError);
	Error(eDLL_T::SERVER, " -> %s\n\n", szContextBuf);
	Error(eDLL_T::SERVER, "%s line [%d] column [%d]\n", pszFile, nLine, nColumn);
}

//---------------------------------------------------------------------------------
// Purpose: prints the global include file the compiler loads for loading scripts
// Input  : *szRsonName - 
//---------------------------------------------------------------------------------
void* HSQVM_LoadRson(const char* szRsonName)
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
	return SQVM_LoadRson(szRsonName);
}

//---------------------------------------------------------------------------------
// Purpose: prints the scripts the compiler loads from global include to be compiled
// Input  : *sqvm - 
//			*szScriptPath - 
//			nFlag - 
//---------------------------------------------------------------------------------
bool HSQVM_LoadScript(void* sqvm, const char* szScriptPath, const char* szScriptName, int nFlag)
{
	if (sq_showscriptloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "Loading SQVM Script '%s'\n", szScriptName);
	}

	///////////////////////////////////////////////////////////////////////////////
	return SQVM_LoadScript(sqvm, szScriptPath, szScriptName, nFlag);
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
void HSQVM_RegisterFunction(void* sqvm, const char* szName, const char* szHelpString, const char* szRetValType, const char* szArgTypes, void* pFunction)
{
	SQFuncRegistration* sqFunc = new SQFuncRegistration();

	sqFunc->m_szScriptName = szName;
	sqFunc->m_szNativeName = szName;
	sqFunc->m_szHelpString = szHelpString;
	sqFunc->m_szRetValType = szRetValType;
	sqFunc->m_szArgTypes   = szArgTypes;
	sqFunc->m_pFunction    = pFunction;

	SQVM_RegisterFunc(sqvm, sqFunc, 1);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void RegisterServerScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "SDKNativeTest", "Native SERVER test function", "void", "", &VSquirrel::SHARED::Script_NativeTest);
}

#ifndef DEDICATED
//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void RegisterClientScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "SDKNativeTest", "Native CLIENT test function", "void", "", &VSquirrel::SHARED::Script_NativeTest);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void RegisterUIScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "SDKNativeTest", "Native UI test function", "void", "", &VSquirrel::SHARED::Script_NativeTest);

	// functions for retrieving server browser data
	HSQVM_RegisterFunction(sqvm, "GetServerName", "Gets the name of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerName);
	HSQVM_RegisterFunction(sqvm, "GetServerPlaylist", "Gets the playlist of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerPlaylist);
	HSQVM_RegisterFunction(sqvm, "GetServerMap", "Gets the map of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerMap);
	HSQVM_RegisterFunction(sqvm, "GetServerCount", "Gets the number of public servers", "int", "", &VSquirrel::UI::GetServerCount);

	// misc main menu functions
	HSQVM_RegisterFunction(sqvm, "GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::UI::GetSDKVersion);
	HSQVM_RegisterFunction(sqvm, "GetPromoData", "Gets promo data for specified slot type", "string", "int", &VSquirrel::UI::GetPromoData);

	// functions for connecting to servers
	HSQVM_RegisterFunction(sqvm, "CreateServer", "Start server with the specified settings", "void", "string,string,string,int", &VSquirrel::UI::CreateServerFromMenu);
	HSQVM_RegisterFunction(sqvm, "SetEncKeyAndConnect", "Set the encryption key to that of the specified server and connects to it", "void", "int", &VSquirrel::UI::SetEncKeyAndConnect);
	HSQVM_RegisterFunction(sqvm, "JoinPrivateServerFromMenu", "Joins private server by token", "void", "string", &VSquirrel::UI::JoinPrivateServerFromMenu);
	HSQVM_RegisterFunction(sqvm, "GetPrivateServerMessage", "Gets private server join status message", "string", "string", &VSquirrel::UI::GetPrivateServerMessage);
	HSQVM_RegisterFunction(sqvm, "ConnectToIPFromMenu", "Joins server by ip and encryption key", "void", "string,string", &VSquirrel::UI::ConnectToIPFromMenu);

	HSQVM_RegisterFunction(sqvm, "GetAvailableMaps", "Gets an array of all the available maps that can be used to host a server", "array<string>", "", &VSquirrel::UI::GetAvailableMaps);
}

//---------------------------------------------------------------------------------
// Purpose: Origin functions are the last to be registered in UI context, we register anything ours below
// Input  : *sqvm - 
// TODO   : Hook 'CreateVM' instead
//---------------------------------------------------------------------------------
void HSQVM_RegisterOriginFuncs(void* sqvm)
{
	if (sqvm == *g_pUIVM.RCast<void**>())
		RegisterUIScriptFunctions(sqvm);
	else
		RegisterClientScriptFunctions(sqvm);

	return SQVM_RegisterOriginFuncs(sqvm);
}
#endif // !DEDICATED

void SQVM_Attach()
{
	DetourAttach((LPVOID*)&SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourAttach((LPVOID*)&SQVM_WarningFunc, &HSQVM_WarningFunc);
	DetourAttach((LPVOID*)&SQVM_ErrorFunc, &HSQVM_ErrorFunc);
	DetourAttach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourAttach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);
#ifndef DEDICATED
	DetourAttach((LPVOID*)&SQVM_RegisterOriginFuncs, &HSQVM_RegisterOriginFuncs);
#endif // !DEDICATED
}

void SQVM_Detach()
{
	DetourDetach((LPVOID*)&SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourDetach((LPVOID*)&SQVM_WarningFunc, &HSQVM_WarningFunc);
	DetourDetach((LPVOID*)&SQVM_ErrorFunc, &HSQVM_ErrorFunc);
	DetourDetach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourDetach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);
#ifndef DEDICATED
	DetourDetach((LPVOID*)&SQVM_RegisterOriginFuncs, &HSQVM_RegisterOriginFuncs);
#endif // !DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
bool g_bSQVM_WarnFuncCalled = false;
