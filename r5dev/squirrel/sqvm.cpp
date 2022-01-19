//=============================================================================//
//
// Purpose: Squirrel VM
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "tier0/basetypes.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "tier0/commandline.h"
#include "engine/sys_utils.h"
#include "vgui/CEngineVGui.h"
#include "gameui/IConsole.h"
#include "squirrel/sqvm.h"
#include "squirrel/sqnativefunctions.h"

//---------------------------------------------------------------------------------
// Purpose: prints the output of each VM to the console
//---------------------------------------------------------------------------------
void* HSQVM_PrintFunc(void* sqvm, char* fmt, ...)
{
#ifdef GAMEDLL_S3
	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18);
#else // TODO [ AMOS ]: nothing equal to 'rdx + 18h' exist in the vm structs for anything below S3.
	static int vmIdx = 3;
#endif
	static char buf[1024] = {};

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
		}
		else
		{
			std::string vmStrAnsi = SQVM_ANSI_LOG_T[vmIdx].c_str();
			vmStrAnsi.append(buf);
			wconsole->debug(vmStrAnsi);
		}

#ifndef DEDICATED
		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();

		iconsole->debug(vmStr);

		std::string s = g_spd_sys_w_oss.str();
		g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));

		if (sq_showvmoutput->GetInt() > 2)
		{
			g_pLogSystem.AddLog((LogType_t)vmIdx, s);
		}
#endif // !DEDICATED
	}
	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: prints the warning output of each VM to the console
//---------------------------------------------------------------------------------
void* HSQVM_WarningFunc(void* sqvm, int a2, int a3, int* nStringSize, void** ppString)
{
	void* result = SQVM_WarningFunc(sqvm, a2, a3, nStringSize, ppString);
	void* retaddr = _ReturnAddress();

	if (retaddr != SQVM_WarningFunc) // Check if its SQVM_Warning calling.
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

	std::string vmStr = SQVM_LOG_T[vmIdx].c_str();
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
		}
		else
		{
			std::string vmStrAnsi = SQVM_ANSI_LOG_T[vmIdx].c_str();
			vmStrAnsi.append(svConstructor);
			wconsole->debug(vmStrAnsi);
		}

#ifndef DEDICATED
		g_spd_sys_w_oss.str("");
		g_spd_sys_w_oss.clear();

		iconsole->debug(vmStr); // Emit to in-game console.

		std::string s = g_spd_sys_w_oss.str();
		g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));

		if (sq_showvmwarning->GetInt() > 2)
		{
			g_pLogSystem.AddLog((LogType_t)vmIdx, s);
			g_pIConsole->m_ivConLog.push_back(Strdup(s.c_str()));
		}
#endif // !DEDICATED
	}
	return result;
}

//---------------------------------------------------------------------------------
// Purpose: loads the include file from the mods directory
//---------------------------------------------------------------------------------
void* HSQVM_LoadRson(const char* szRsonName)
{
	char szFilePath[MAX_PATH] = { 0 };
	sprintf_s(szFilePath, MAX_PATH, "platform\\%s", szRsonName);

	// Flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(szFilePath); i++)
	{
		if (szFilePath[i] == '/')
		{
			szFilePath[i] = '\\';
		}
	}

	// Returns the new path if the rson exists on the disk
	if (FileExists(szFilePath) && SQVM_LoadRson(szRsonName))
	{
		if (sq_showrsonloading->GetBool())
		{
			DevMsg(eDLL_T::ENGINE, "\n");
			DevMsg(eDLL_T::ENGINE, "______________________________________________________________\n");
			DevMsg(eDLL_T::ENGINE, "] RSON_DISK --------------------------------------------------\n");
			DevMsg(eDLL_T::ENGINE, "] PATH: '%s'\n", szFilePath);
			DevMsg(eDLL_T::ENGINE, "--------------------------------------------------------------\n");
			DevMsg(eDLL_T::ENGINE, "\n");
		}
		return SQVM_LoadRson(szFilePath);
	}
	else
	{
		if (sq_showrsonloading->GetBool())
		{
			DevMsg(eDLL_T::ENGINE, "\n");
			DevMsg(eDLL_T::ENGINE, "______________________________________________________________\n");
			DevMsg(eDLL_T::ENGINE, "] RSON_VPK ---------------------------------------------------\n");
			DevMsg(eDLL_T::ENGINE, "] PATH: '%s'\n", szRsonName);
			DevMsg(eDLL_T::ENGINE, "--------------------------------------------------------------\n");
			DevMsg(eDLL_T::ENGINE, "\n");
		}
	}
	return SQVM_LoadRson(szRsonName);
}

//---------------------------------------------------------------------------------
// Purpose: loads the script file from the mods directory
//---------------------------------------------------------------------------------
bool HSQVM_LoadScript(void* sqvm, const char* szScriptPath, const char* szScriptName, int nFlag)
{
	char filepath[MAX_PATH] = { 0 };
	sprintf_s(filepath, MAX_PATH, "platform\\%s", szScriptPath);

	// Flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(filepath); i++)
	{
		if (filepath[i] == '/')
		{
			filepath[i] = '\\';
		}
	}

	if (sq_showscriptloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "Loading SQVM Script '%s'\n", filepath);
	}

	// Returns true if the script exists on the disk
	if (FileExists(filepath) && SQVM_LoadScript(sqvm, filepath, szScriptName, nFlag))
	{
		return true;
	}

	if (sq_showscriptloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "FAILED. Try SP / VPK for '%s'\n", filepath);
	}

	///////////////////////////////////////////////////////////////////////////////
	return SQVM_LoadScript(sqvm, szScriptPath, szScriptName, nFlag);
}

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

int HSQVM_NativeTest(void* sqvm)
{
	// Function code goes here.
	return 1;
}

void RegisterUIScriptFunctions(void* sqvm)
{
#ifndef DEDICATED
	HSQVM_RegisterFunction(sqvm, "UINativeTest", "native ui test function", "void", "", &HSQVM_NativeTest);

	// functions for retrieving server browser data
	HSQVM_RegisterFunction(sqvm, "GetServerName", "get name of the server at the specified index of the server list", "string", "int", &SQNativeFunctions::IBrowser::GetServerName);
	HSQVM_RegisterFunction(sqvm, "GetServerDescription", "get description of the server at the specified index of the server list", "string", "int", &SQNativeFunctions::IBrowser::GetServerDescription);
	HSQVM_RegisterFunction(sqvm, "GetServerPlaylist", "get playlist of the server at the specified index of the server list", "string", "int", &SQNativeFunctions::IBrowser::GetServerPlaylist);
	HSQVM_RegisterFunction(sqvm, "GetServerMap", "get map of the server at the specified index of the server list", "string", "int", &SQNativeFunctions::IBrowser::GetServerMap);
	HSQVM_RegisterFunction(sqvm, "GetServerPlayersNum", "get num of connected players of the server at the specified index of the server list", "int", "int", &SQNativeFunctions::IBrowser::GetServerPlayersNum);
	HSQVM_RegisterFunction(sqvm, "GetServerMaxPlayersNum", "get num of max players of the server at the specified index of the server list", "int", "int", &SQNativeFunctions::IBrowser::GetServerMaxPlayersNum);
	HSQVM_RegisterFunction(sqvm, "GetServerCount", "get number of public servers", "int", "", &SQNativeFunctions::IBrowser::GetServerCount);


	// misc main menu functions
	HSQVM_RegisterFunction(sqvm, "GetSDKVersion", "get sdk version as a string", "string", "", &SQNativeFunctions::IBrowser::GetSDKVersion);
	HSQVM_RegisterFunction(sqvm, "GetPromoData", "get promo data for specified slot type", "string", "int", &SQNativeFunctions::IBrowser::GetPromoData);

	// functions for connecting to servers
	HSQVM_RegisterFunction(sqvm, "CreateServer", "start server with the specified settings", "void", "string,string,string,string,string", &SQNativeFunctions::IBrowser::CreateServerFromMenu);
	HSQVM_RegisterFunction(sqvm, "SetEncKeyAndConnect", "set the encryption key to that of the specified server and connects to it", "void", "int", &SQNativeFunctions::IBrowser::SetEncKeyAndConnect);
	HSQVM_RegisterFunction(sqvm, "JoinPrivateServerFromMenu", "join private server by token", "void", "string", &SQNativeFunctions::IBrowser::JoinPrivateServerFromMenu);
	HSQVM_RegisterFunction(sqvm, "GetPrivateServerMessage", "get private server join status message", "string", "string", &SQNativeFunctions::IBrowser::GetPrivateServerMessage);
	HSQVM_RegisterFunction(sqvm, "ConnectToIPFromMenu", "join server by ip and encryption key", "void", "string,string", &SQNativeFunctions::IBrowser::ConnectToIPFromMenu);

	HSQVM_RegisterFunction(sqvm, "GetAvailableMaps", "gets an array of all the available maps that can be used to host a server", "array<string>", "", &SQNativeFunctions::IBrowser::GetAvailableMaps);
#endif
}

void RegisterClientScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "ClientNativeTest", "native client function", "void", "", &HSQVM_NativeTest);
}

void RegisterServerScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "ServerNativeTest", "native server function", "void", "", &HSQVM_NativeTest);
}

void HSQVM_RegisterOriginFuncs(void* sqvm)
{
	if (sqvm == *p_SQVM_UIVM.RCast<void**>())
		RegisterUIScriptFunctions(sqvm);
	else
		RegisterClientScriptFunctions(sqvm);

	return SQVM_RegisterOriginFuncs(sqvm);
}

void SQVM_Attach()
{
	DetourAttach((LPVOID*)&SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourAttach((LPVOID*)&SQVM_WarningFunc, &HSQVM_WarningFunc);
	DetourAttach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourAttach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);
	DetourAttach((LPVOID*)&SQVM_RegisterOriginFuncs, &HSQVM_RegisterOriginFuncs);
}

void SQVM_Detach()
{
	DetourDetach((LPVOID*)&SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourDetach((LPVOID*)&SQVM_WarningFunc, &HSQVM_WarningFunc);
	DetourDetach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourDetach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);
	DetourDetach((LPVOID*)&SQVM_RegisterOriginFuncs, &HSQVM_RegisterOriginFuncs);
}

///////////////////////////////////////////////////////////////////////////////
bool g_bSQVM_WarnFuncCalled = false;
