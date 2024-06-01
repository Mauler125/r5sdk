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

		SQRESULT GetNumHumanPlayers(HSQUIRRELVM v);
		SQRESULT GetNumFakeClients(HSQUIRRELVM v);

		SQRESULT GetServerID(HSQUIRRELVM v);

		SQRESULT IsServerActive(HSQUIRRELVM v);
		SQRESULT IsDedicated(HSQUIRRELVM v);
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
