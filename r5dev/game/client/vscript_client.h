#ifndef VSCRIPT_CLIENT_H
#define VSCRIPT_CLIENT_H

namespace VScriptCode
{
	namespace Client
	{
		SQRESULT RefreshServerCount(HSQUIRRELVM v);
		SQRESULT GetServerCount(HSQUIRRELVM v);

		SQRESULT GetHiddenServerName(HSQUIRRELVM v);
		SQRESULT GetServerName(HSQUIRRELVM v);
		SQRESULT GetServerDescription(HSQUIRRELVM v);

		SQRESULT GetServerMap(HSQUIRRELVM v);
		SQRESULT GetServerPlaylist(HSQUIRRELVM v);

		SQRESULT GetServerCurrentPlayers(HSQUIRRELVM v);
		SQRESULT GetServerMaxPlayers(HSQUIRRELVM v);

		SQRESULT GetPromoData(HSQUIRRELVM v);

		SQRESULT ConnectToListedServer(HSQUIRRELVM v);
		SQRESULT ConnectToHiddenServer(HSQUIRRELVM v);
		SQRESULT ConnectToServer(HSQUIRRELVM v);

		SQRESULT IsClientDLL(HSQUIRRELVM v);
	}
}

void Script_RegisterClientFunctions(CSquirrelVM* s);
void Script_RegisterUIFunctions(CSquirrelVM* s);
void Script_RegisterCoreClientFunctions(CSquirrelVM* s);

#endif // VSCRIPT_CLIENT_H
