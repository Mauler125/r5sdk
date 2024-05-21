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

		SQRESULT InitializeLogThread__internal(HSQUIRRELVM v);
		SQRESULT LogEvent__internal(HSQUIRRELVM v);
		SQRESULT SQMatchID__internal(HSQUIRRELVM v);
		SQRESULT stopLogging__internal(HSQUIRRELVM v);
		SQRESULT isLogging__internal(HSQUIRRELVM v);
		SQRESULT SQ_GetLogState__internal(HSQUIRRELVM v);


		SQRESULT sqprint(HSQUIRRELVM v);
		SQRESULT sqerror(HSQUIRRELVM v);

		SQRESULT EA_Verify__internal(HSQUIRRELVM v);
		SQRESULT _STATSHOOK_UpdatePlayerCount__internal(HSQUIRRELVM v);
		SQRESULT _STATSHOOK_EndOfMatch__internal(HSQUIRRELVM v);

		SQRESULT LoadSyncData__internal(HSQUIRRELVM v);
		SQRESULT GetSyncData__internal(HSQUIRRELVM v);

		SQRESULT SQ_UpdateLiveStats__internal(HSQUIRRELVM v);
		SQRESULT SQ_ResetStats__internal(HSQUIRRELVM v);

		SQRESULT LoadBatchSyncData__internal(HSQUIRRELVM v);

		SQRESULT CleanupLogs__internal(HSQUIRRELVM v);

		SQRESULT SQ_GetSetting__internal(HSQUIRRELVM v); //DEPRECATED
		SQRESULT GetPlayerStats__internal(HSQUIRRELVM v); //NEW

		SQRESULT SQ_ReloadConfig__internal(HSQUIRRELVM v);
		SQRESULT FetchGlobalSettingsFromR5RDEV__internal(HSQUIRRELVM v);

		SQRESULT SQ_ServerMsg__internal(HSQUIRRELVM v);
		SQRESULT SQ_CreateServerBot__internal(HSQUIRRELVM v);

		SQRESULT PrintStack(HSQUIRRELVM v);

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
