#ifndef VSCRIPT_SERVER_H
#define VSCRIPT_SERVER_H
#include "vscript/languages/squirrel_re/vsquirrel.h"

namespace VScriptCode
{
	namespace Server
	{
		SQRESULT CreateServer(HSQUIRRELVM v);
		SQRESULT DestroyServer(HSQUIRRELVM v);

		SQRESULT SetAutoReloadState(HSQUIRRELVM v);

		SQRESULT KickPlayerByName(HSQUIRRELVM v);
		SQRESULT KickPlayerById(HSQUIRRELVM v);
		SQRESULT BanPlayerByName(HSQUIRRELVM v);
		SQRESULT BanPlayerById(HSQUIRRELVM v);
		SQRESULT UnbanPlayer(HSQUIRRELVM v);
		SQRESULT AddBanByID(HSQUIRRELVM v);

		SQRESULT GetNumHumanPlayers(HSQUIRRELVM v);
		SQRESULT GetNumFakeClients(HSQUIRRELVM v);

		SQRESULT GetServerID(HSQUIRRELVM v);

		SQRESULT IsServerActive(HSQUIRRELVM v);
		SQRESULT IsDedicated(HSQUIRRELVM v);

		SQRESULT InitializeLogThread_internal(HSQUIRRELVM v);
		SQRESULT LogEvent(HSQUIRRELVM v);
		SQRESULT SQMatchID(HSQUIRRELVM v);
		SQRESULT stopLogging(HSQUIRRELVM v);
		SQRESULT isLogging(HSQUIRRELVM v);
		SQRESULT SQ_GetLogState(HSQUIRRELVM v);


		SQRESULT sqprint(HSQUIRRELVM v);
		SQRESULT sqerror(HSQUIRRELVM v);

		SQRESULT EA_Verify(HSQUIRRELVM v);
		SQRESULT _STATSHOOK_UpdatePlayerCount(HSQUIRRELVM v);
		SQRESULT _STATSHOOK_EndOfMatch(HSQUIRRELVM v);
		SQRESULT LoadKDString(HSQUIRRELVM v);
		SQRESULT GetKDString(HSQUIRRELVM v);
		SQRESULT SQ_UpdateLiveStats(HSQUIRRELVM v);
		SQRESULT SQ_ResetStats(HSQUIRRELVM v);
		SQRESULT LoadBatchKDStrings(HSQUIRRELVM v);
		SQRESULT CleanupLogs(HSQUIRRELVM v);
		SQRESULT SQ_GetSetting(HSQUIRRELVM v);
		SQRESULT SQ_ReloadConfig(HSQUIRRELVM v);
		SQRESULT FetchGlobalSettingsFromR5RDEV(HSQUIRRELVM v);


		SQRESULT SQ_ServerMsg(HSQUIRRELVM v);
		SQRESULT SQ_CreateServerBot(HSQUIRRELVM v);


		//Declare to be used with logger
		int64_t getMatchID();
		void setMatchID(int64_t newID);
	}
}

void Script_RegisterServerFunctions(CSquirrelVM* s);
void Script_RegisterCoreServerFunctions(CSquirrelVM* s);
void Script_RegisterAdminPanelFunctions(CSquirrelVM* s);

void Script_RegisterServerEnums(CSquirrelVM* const s);

#define DEFINE_SERVER_SCRIPTFUNC_NAMED(s, functionName, helpString,     \
	returnType, parameters)                                             \
	s->RegisterFunction(#functionName, MKSTRING(Script_##functionName), \
	helpString, returnType, parameters, VScriptCode::Server::##functionName);   \

#endif // VSCRIPT_SERVER_H
