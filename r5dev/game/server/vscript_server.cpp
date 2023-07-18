//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// See 'game/shared/vscript_shared.cpp' for more details.
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/server/server.h"
#include "game/shared/vscript_shared.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

namespace VScriptCode
{
    namespace Server
    {
        //-----------------------------------------------------------------------------
        // Purpose: gets the number of real players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumHumanPlayers(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumHumanPlayers());
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the number of fake players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumFakeClients(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumFakeClients());
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: checks whether this SDK build is a dedicated server
        //-----------------------------------------------------------------------------
        SQRESULT IsDedicated(HSQUIRRELVM v)
        {
            sq_pushbool(v, ::IsDedicated());
            return SQ_OK;
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterServerFunctions(CSquirrelVM* s)
{
    s->RegisterFunction("SDKNativeTest", "Script_SDKNativeTest", "Native SERVER test function", "void", "", &VScriptCode::Shared::SDKNativeTest);
    s->RegisterFunction("GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VScriptCode::Shared::GetSDKVersion);

    s->RegisterFunction("GetNumHumanPlayers", "Script_GetNumHumanPlayers", "Gets the number of human players on the server", "int", "", &VScriptCode::Server::GetNumHumanPlayers);
    s->RegisterFunction("GetNumFakeClients", "Script_GetNumFakeClients", "Gets the number of bot players on the server", "int", "", &VScriptCode::Server::GetNumFakeClients);

    s->RegisterFunction("GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VScriptCode::Shared::GetAvailableMaps);
    s->RegisterFunction("GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VScriptCode::Shared::GetAvailablePlaylists);

    s->RegisterFunction("KickPlayerByName", "Script_KickPlayerByName", "Kicks a player from the server by name", "void", "string, string", &VScriptCode::Shared::KickPlayerByName);
    s->RegisterFunction("KickPlayerById", "Script_KickPlayerById", "Kicks a player from the server by handle or nucleus id", "void", "string, string", &VScriptCode::Shared::KickPlayerById);

    s->RegisterFunction("BanPlayerByName", "Script_BanPlayerByName", "Bans a player from the server by name", "void", "string", &VScriptCode::Shared::BanPlayerByName);
    s->RegisterFunction("BanPlayerById", "Script_BanPlayerById", "Bans a player from the server by handle or nucleus id", "void", "string, string", &VScriptCode::Shared::BanPlayerById);

    s->RegisterFunction("UnbanPlayer", "Script_UnbanPlayer", "Unbans a player from the server by nucleus id or ip address", "void", "string, string", &VScriptCode::Shared::UnbanPlayer);

    s->RegisterFunction("ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VScriptCode::Shared::ShutdownHostGame);

    s->RegisterFunction("IsDedicated", "Script_IsDedicated", "Returns whether this is a dedicated server", "bool", "", &VScriptCode::Server::IsDedicated);
}
