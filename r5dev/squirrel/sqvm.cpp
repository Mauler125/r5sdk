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
#include "engine/sys_utils.h"
#include "squirrel/sqvm.h"
#include "vgui/CEngineVGui.h"
#include "gameui/IConsole.h"
#include <squirrel/sqapi.h>
#include <networksystem/r5net.h>
#include "gameui/IBrowser.h"
#include <engine/net_chan.h>
#include <client/IVEngineClient.h>
#include "vpc/keyvalues.h"
#include <windows/id3dx.h>

//---------------------------------------------------------------------------------
// Purpose: prints the output of each VM to the console
//---------------------------------------------------------------------------------
void* HSQVM_PrintFunc(void* sqvm, char* fmt, ...)
{
#ifdef GAMEDLL_S3
	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18);
#else // TODO [ AMOS ]: nothing equal to 'rdx + 18h' exist in the vm pointers for anything below S3.
	int vmIdx = 3;
#endif
	static bool initialized = false;

	static char buf[1024];
	static std::string vmType[4] = { "Script(S):", "Script(C):", "Script(U):", "Script(X):" };

	static auto iconsole = spdlog::stdout_logger_mt("sqvm_print_iconsole"); // in-game console.
	static auto wconsole = spdlog::stdout_logger_mt("sqvm_print_wconsole"); // windows console.
	static auto sqlogger = spdlog::basic_logger_mt("sqvm_print_logger", "platform\\logs\\sqvm_print.log"); // file logger.

	std::string vmStr = vmType[vmIdx].c_str();

	g_spd_sqvm_p_oss.str("");
	g_spd_sqvm_p_oss.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("sqvm_print_ostream", g_spd_sqvm_p_ostream_sink);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v");
		wconsole->set_level(spdlog::level::debug);
		sqlogger->set_pattern("[%S.%e] %v");
		sqlogger->set_level(spdlog::level::debug);
		initialized = true;
	}

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	vmStr.append(buf);

	if (sq_showvmoutput->GetInt() > 0)
	{
		sqlogger->debug(vmStr);
	}
	if (sq_showvmoutput->GetInt() > 1)
	{
		iconsole->debug(vmStr);
		wconsole->debug(vmStr);
#ifndef DEDICATED
		std::string s = g_spd_sqvm_p_oss.str();
		const char* c = s.c_str();
		g_pIConsole->m_ivConLog.push_back(Strdup((const char*)c));
#endif // !DEDICATED
	}
#ifndef DEDICATED
	if (sq_showvmoutput->GetInt() > 2)
	{
		std::string s = g_spd_sqvm_p_oss.str();
		g_pLogSystem.AddLog((LogType_t)vmIdx, s);
	}
#endif // !DEDICATED
	return NULL;
}

//---------------------------------------------------------------------------------
// Purpose: prints the warning output of each VM to the console
//---------------------------------------------------------------------------------
void* HSQVM_WarningFunc(void* sqvm, int a2, int a3, int* nStringSize, void** ppString)
{
	void* result = SQVM_WarningFunc(sqvm, a2, a3, nStringSize, ppString);
	if (g_bSQVM_WarnFuncCalled) // Check if its SQVM_Warning calling.
	{
		return result; // If not return.
	}

	static bool initialized = false;
	static std::string vmType[4] = { "Script(S): WARNING: ", "Script(C): WARNING: ", "Script(U): WARNING: ", "Script(X): WARNING: " };

	static auto iconsole = spdlog::stdout_logger_mt("sqvm_warn_iconsole"); // in-game console.
	static auto wconsole = spdlog::stdout_logger_mt("sqvm_warn_wconsole"); // windows console.
	static auto sqlogger = spdlog::basic_logger_mt("sqvm_warn_logger", "platform\\logs\\sqvm_warn.log"); // file logger.

#ifdef GAMEDLL_S3
	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18);
#else // TODO [ AMOS ]: nothing equal to 'rdx + 18h' exist in the vm pointers for anything below S3.
	int vmIdx = 3;
#endif
	std::string vmStr = vmType[vmIdx].c_str();

	g_spd_sqvm_w_oss.str("");
	g_spd_sqvm_w_oss.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("sqvm_warn_ostream", g_spd_sqvm_p_ostream_sink);
		iconsole->set_pattern("[%S.%e] %v\n");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v\n");
		wconsole->set_level(spdlog::level::debug);
		sqlogger->set_pattern("[%S.%e] %v\n");
		sqlogger->set_level(spdlog::level::debug);
		initialized = true;
	}

	std::string stringConstructor((char*)*ppString, *nStringSize); // Get string from memory via std::string constructor.
	vmStr.append(stringConstructor);

	std::string s = g_spd_sqvm_w_oss.str();
	const char* c = s.c_str();

	if (sq_showvmwarning->GetInt() > 0)
	{
		sqlogger->debug(vmStr); // Emit to file.
	}
	if (sq_showvmwarning->GetInt() > 1)
	{
		iconsole->debug(vmStr); // Emit to in-game console.
		wconsole->debug(vmStr); // Emit to windows console.
#ifndef DEDICATED
		std::string s = g_spd_sqvm_w_oss.str();
		const char* c = s.c_str();
		g_pIConsole->m_ivConLog.push_back(Strdup(c));
#endif // !DEDICATED
	}
#ifndef DEDICATED
	if (sq_showvmwarning->GetInt() > 2)
	{
		g_pLogSystem.AddLog((LogType_t)vmIdx, s);
		const char* c = s.c_str();
		g_pIConsole->m_ivConLog.push_back(Strdup(c));
	}
#endif // !DEDICATED
	g_bSQVM_WarnFuncCalled = false;

	return result;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void* HSQVM_WarningCmd(int a1, int a2)
{
	g_bSQVM_WarnFuncCalled = true;
	return SQVM_WarningCmd(a1, a2);
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

std::vector<ServerListing> ServerList;

int GetServerName(void* sqvm)
{

	int a = hsq_getinteger(sqvm, 1);
	std::string str = ServerList[a].svServerName;

	char* c = const_cast<char*>(str.c_str());

	hsq_pushstring(sqvm, c, -1);

	return 1;
}

int GetServerPlaylist(void* sqvm)
{
	int a = hsq_getinteger(sqvm, 1);
	std::string str = ServerList[a].svPlaylist;

	char* c = const_cast<char*>(str.c_str());

	hsq_pushstring(sqvm, c, -1);

	return 1;
}

int GetServerMap(void* sqvm)
{
	int a = hsq_getinteger(sqvm, 1);
	std::string str = ServerList[a].svMapName;

	char* c = const_cast<char*>(str.c_str());

	hsq_pushstring(sqvm, c, -1);

	return 1;
}

int GetServerAmmount(void* sqvm)
{
	std::string msg;
	ServerList = g_pR5net->GetServersList(msg);

	int i = -1;

	for (ServerListing& server : ServerList)
	{
		i++;
	}

	hsq_pushinteger(sqvm, i);

	return 1;
}

int GetSDKVersion(void* sqvm)
{
	std::string SDKV;
	SDKV = g_pR5net->GetSDKVersion();

	char* c = const_cast<char*>(SDKV.c_str());

	hsq_pushstring(sqvm, c, -1);

	return 1;
}

int GetPromoData(void* sqvm)
{
	//PromoLargeTitle, PromoLargeDesc, PromoLeftTitle, PromoLeftDesc, PromoRightTitle, PromoRightDesc
	std::string promodatatype = hsq_getstring(sqvm, 1);
	std::string senddataback;

	if (promodatatype == "PromoLargeTitle") {
		senddataback = "Welcome To R5Reloaded!";
	}
	else if (promodatatype == "PromoLargeDesc") {
		senddataback = "Make sure to join the discord! discord.gg/r5reloaded";
	}
	else if (promodatatype == "PromoLeftTitle") {
		senddataback = "Check out the Rendy Gaming montage";
	}
	else if (promodatatype == "PromoLeftDesc") {
		senddataback = "This can be hooked up to recive data from a server. Check sqvm.cpp";
	}
	else if (promodatatype == "PromoRightTitle") {
		senddataback = "Check out the Rendy Gaming montage";
	}
	else if (promodatatype == "PromoRightDesc") {
		senddataback = "This can be hooked up to recive data from a server. Check sqvm.cpp";
	}
	else
	{
		return 1;
	}

	char* c = const_cast<char*>(senddataback.c_str());

	hsq_pushstring(sqvm, c, -1);

	return 1;
}

int SetEncKeyAndConnect(void* sqvm)
{
	int i = hsq_getinteger(sqvm, 1);

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Avoid race condition.

	HNET_SetKey(ServerList[i].svEncryptionKey);

	std::stringstream cmd;
	cmd << "connect " << ServerList[i].svIpAddress << ":" << ServerList[i].svPort;

	//This dosnt crash
	IVEngineClient_CommandExecute(NULL, cmd.str().c_str());

	//This crashes becuase the server browser needs to be initilized at leats once before i can send the connect command
	//g_pServerBrowser->ConnectToServer(ServerList[i].svIpAddress, ServerList[i].svPort, ServerList[i].svEncryptionKey);

	return 1;
}

int CreateServerFromMenu(void* sqvm)
{
	std::string servername = hsq_getstring(sqvm, 1);
	std::string mapname = hsq_getstring(sqvm, 2);
	std::string gamemode = hsq_getstring(sqvm, 3);
	std::string vis = hsq_getstring(sqvm, 4);

	if (mapname == "")
		return 1;

	if (gamemode == "")
		return 1;

	if (vis == "")
		return 1;

	//Had to do a shitty fix in scripts to get this to work maybe someone else can do a better job
	//server browser needs to be opened at least once to set the vars when creating a server
	g_pIBrowser->SetMenuVars(servername, vis);

	DevMsg(eDLL_T::ENGINE, "Starting Server with map '%s' and playlist '%s'\n", mapname.c_str(), gamemode.c_str());

	KeyValues_LoadPlaylist(gamemode.c_str());
	std::stringstream cgmd;
	cgmd << "mp_gamemode " << gamemode;
	IVEngineClient_CommandExecute(NULL, cgmd.str().c_str());

	// This is to avoid a race condition.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::stringstream cmd;
	cmd << "map " << mapname;
	IVEngineClient_CommandExecute(NULL, cmd.str().c_str());

	return 1;
}

int JoinPrivateServerFromMenu(void* sqvm)
{
	std::string m_szHiddenServerRequestMessage;

	std::string token = hsq_getstring(sqvm, 1);

	ServerListing server;
	bool result = g_pR5net->GetServerByToken(server, m_szHiddenServerRequestMessage, token); // Send token connect request.
	if (!server.svServerName.empty())
	{
		HNET_SetKey(server.svEncryptionKey);

		std::stringstream cmd;
		cmd << "connect " << server.svIpAddress << ":" << server.svPort;

		//This dosnt crash
		IVEngineClient_CommandExecute(NULL, cmd.str().c_str());
	}

	return 1;
}

int GetPrivateServerMessage(void* sqvm)
{
	std::string m_szHiddenServerRequestMessage;

	std::string token = hsq_getstring(sqvm, 1);

	ServerListing server;
	bool result = g_pR5net->GetServerByToken(server, m_szHiddenServerRequestMessage, token); // Send token connect request.
	if (!server.svServerName.empty())
	{
		m_szHiddenServerRequestMessage = "Found Server: " + server.svServerName;

		char* c = const_cast<char*>(m_szHiddenServerRequestMessage.c_str());

		hsq_pushstring(sqvm, c, -1);
	}
	else
	{
		m_szHiddenServerRequestMessage = "Error: Server Not Found";

		char* c = const_cast<char*>(m_szHiddenServerRequestMessage.c_str());

		hsq_pushstring(sqvm, c, -1);
	}

	return 1;
}

int ConnectToIPFromMenu(void* sqvm)
{
	std::string ip = hsq_getstring(sqvm, 1);
	std::string key = hsq_getstring(sqvm, 2);

	if (key != "")
	{
		HNET_SetKey(key);
	}

	std::stringstream cmd;
	cmd << "connect " << ip;

	//This dosnt crash
	IVEngineClient_CommandExecute(NULL, cmd.str().c_str());

	return 1;
}

void RegisterUIScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "UINativeTest", "native ui function", "void", "", &HSQVM_NativeTest);

	//Server Browser Data
	HSQVM_RegisterFunction(sqvm, "GetServerName", "native ui function", "string", "int", &GetServerName);
	HSQVM_RegisterFunction(sqvm, "GetServerPlaylist", "native ui function", "string", "int", &GetServerPlaylist);
	HSQVM_RegisterFunction(sqvm, "GetServerMap", "native ui function", "string", "int", &GetServerMap);
	HSQVM_RegisterFunction(sqvm, "GetServerAmmount", "native ui function", "int", "", &GetServerAmmount);

	//Main Menu Data
	HSQVM_RegisterFunction(sqvm, "GetSDKVersion", "native ui function", "string", "", &GetSDKVersion);
	HSQVM_RegisterFunction(sqvm, "GetPromoData", "native ui function", "string", "string", &GetPromoData);

	//Connecting To Servers
	HSQVM_RegisterFunction(sqvm, "CreateServer", "native ui function", "void", "string,string,string,string", &CreateServerFromMenu);
	HSQVM_RegisterFunction(sqvm, "SetEncKeyAndConnect", "native ui function", "void", "int", &SetEncKeyAndConnect);
	HSQVM_RegisterFunction(sqvm, "JoinPrivateServerFromMenu", "native ui function", "void", "string", &JoinPrivateServerFromMenu);
	HSQVM_RegisterFunction(sqvm, "GetPrivateServerMessage", "native ui function", "string", "string", &GetPrivateServerMessage);
	HSQVM_RegisterFunction(sqvm, "ConnectToIPFromMenu", "native ui function", "void", "string,string", &ConnectToIPFromMenu);
}

void RegisterClientScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "ClientNativeTest", "native client function", "void", "", &HSQVM_NativeTest);
}

void RegisterServerScriptFunctions(void* sqvm)
{
	HSQVM_RegisterFunction(sqvm, "ServerNativeTest", "native server function", "void", "", &HSQVM_NativeTest);
}

ADDRESS UIVM = (void*)p_SQVM_CreateUIVM.FollowNearCall().FindPatternSelf("48 8B 1D", ADDRESS::Direction::DOWN, 50).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr();

void HSQVM_RegisterOriginFuncs(void* sqvm)
{
	if (sqvm == *UIVM.RCast<void**>())
		RegisterUIScriptFunctions(sqvm);
	else
		RegisterClientScriptFunctions(sqvm);
	return SQVM_RegisterOriginFuncs(sqvm);
}


void SQVM_Attach()
{
	DetourAttach((LPVOID*)&SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourAttach((LPVOID*)&SQVM_WarningFunc, &HSQVM_WarningFunc);
	DetourAttach((LPVOID*)&SQVM_WarningCmd, &HSQVM_WarningCmd);
	DetourAttach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourAttach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);
	DetourAttach((LPVOID*)&SQVM_RegisterOriginFuncs, &HSQVM_RegisterOriginFuncs);
}

void SQVM_Detach()
{
	DetourDetach((LPVOID*)&SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourDetach((LPVOID*)&SQVM_WarningFunc, &HSQVM_WarningFunc);
	DetourDetach((LPVOID*)&SQVM_WarningCmd, &HSQVM_WarningCmd);
	DetourDetach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourDetach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);
	DetourDetach((LPVOID*)&SQVM_RegisterOriginFuncs, &HSQVM_RegisterOriginFuncs);
}

///////////////////////////////////////////////////////////////////////////////
bool g_bSQVM_WarnFuncCalled = false;
