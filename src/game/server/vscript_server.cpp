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
#include "vscript/vscript.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

#include "liveapi/liveapi.h"
#include "vscript_server.h"
#include <engine/host_state.h>
#include <networksystem/hostmanager.h>

/*
=====================
SQVM_ServerScript_f

  Executes input on the
  VM in SERVER context.
=====================
*/
static void SQVM_ServerScript_f(const CCommand& args)
{
    if (args.ArgC() >= 2)
    {
        Script_Execute(args.ArgS(), SQCONTEXT::SERVER);
    }
}
static ConCommand script("script", SQVM_ServerScript_f, "Run input code as SERVER script on the VM", FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_SERVER_FRAME_THREAD);

namespace VScriptCode
{
    namespace Server
    {
        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        // TODO: return a boolean on failure instead of raising an error, so we could
        // determine from scripts whether or not to spin a local server, or connect
        // to a dedicated server (for disconnecting and loading the lobby, for example)
        //-----------------------------------------------------------------------------
        SQRESULT CreateServer(HSQUIRRELVM v)
        {
            SQChar* serverName = sq_getstring(v, 1);
            SQChar* serverDescription = sq_getstring(v, 2);
            SQChar* serverMapName = sq_getstring(v, 3);
            SQChar* serverPlaylist = sq_getstring(v, 4);
            ServerVisibility_e serverVisibility = static_cast<ServerVisibility_e>(sq_getinteger(v, 5));

            if (!VALID_CHARSTAR(serverName) ||
                !VALID_CHARSTAR(serverMapName) ||
                !VALID_CHARSTAR(serverPlaylist))
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
            }

            // Adjust browser settings.
            NetGameServer_t& details = g_ServerHostManager.GetDetails();

            details.name = serverName;
            details.description = serverDescription;
            details.map = serverMapName;
            details.playlist = serverPlaylist;

            // Launch server.
            g_ServerHostManager.SetVisibility(serverVisibility);
            g_ServerHostManager.LaunchServer(g_pServer->IsActive());

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }
        //-----------------------------------------------------------------------------
        // Purpose: shuts the server down and disconnects all clients
        //-----------------------------------------------------------------------------
        SQRESULT DestroyServer(HSQUIRRELVM v)
        {
            if (g_pHostState->m_bActiveGame)
                g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerByName(HSQUIRRELVM v)
        {
            SQChar* playerName = sq_getstring(v, 1);
            SQChar* reason = sq_getstring(v, 2);

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.KickPlayerByName(playerName, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given handle or id
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerById(HSQUIRRELVM v)
        {
            SQChar* playerHandle = sq_getstring(v, 1);
            SQChar* reason = sq_getstring(v, 2);

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.KickPlayerById(playerHandle, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: bans a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT BanPlayerByName(HSQUIRRELVM v)
        {
            SQChar* playerName = sq_getstring(v, 1);
            SQChar* reason = sq_getstring(v, 2);

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.BanPlayerByName(playerName, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: bans a player by given handle or id
        //-----------------------------------------------------------------------------
        SQRESULT BanPlayerById(HSQUIRRELVM v)
        {
            SQChar* playerHandle = sq_getstring(v, 1);
            SQChar* reason = sq_getstring(v, 2);

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.BanPlayerById(playerHandle, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: unbans a player by given nucleus id or ip address
        //-----------------------------------------------------------------------------
        SQRESULT UnbanPlayer(HSQUIRRELVM v)
        {
            SQChar* szCriteria = sq_getstring(v, 1);
            g_BanSystem.UnbanPlayer(szCriteria);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the number of real players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumHumanPlayers(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumHumanPlayers());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the number of fake players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumFakeClients(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumFakeClients());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: checks whether the server is active
        //-----------------------------------------------------------------------------
        SQRESULT IsServerActive(HSQUIRRELVM v)
        {
            bool isActive = g_pServer->IsActive();
            sq_pushbool(v, isActive);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: checks whether this SDK build is a dedicated server
        //-----------------------------------------------------------------------------
        SQRESULT IsDedicated(HSQUIRRELVM v)
        {
            sq_pushbool(v, ::IsDedicated());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterServerFunctions(CSquirrelVM* s)
{
    Script_RegisterCommonAbstractions(s);
    Script_RegisterCoreServerFunctions(s);
    Script_RegisterAdminPanelFunctions(s);

    Script_RegisterLiveAPIFunctions(s);
}

void Script_RegisterServerEnums(CSquirrelVM* const s)
{
    Script_RegisterLiveAPIEnums(s);
}

//---------------------------------------------------------------------------------
// Purpose: core server script functions
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCoreServerFunctions(CSquirrelVM* s)
{
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, IsServerActive, "Returns whether the server is active", "bool", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, IsDedicated, "Returns whether this is a dedicated server", "bool", "");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, CreateServer, "Starts server with the specified settings", "void", "string, string, string, string, int");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, DestroyServer, "Shuts the local server down", "void", "");
}

//---------------------------------------------------------------------------------
// Purpose: admin panel script functions
// Input  : *s - 
// 
// Ideally, these get dropped entirely in favor of remote functions. Currently,
// the s3 build only supports remote function calls from server to client/ui.
// Client/ui to server is all done through clientcommands.
//---------------------------------------------------------------------------------
void Script_RegisterAdminPanelFunctions(CSquirrelVM* s)
{
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetNumHumanPlayers, "Gets the number of human players on the server", "int", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetNumFakeClients, "Gets the number of bot players on the server", "int", "");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, KickPlayerByName, "Kicks a player from the server by name", "void", "string, string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, KickPlayerById, "Kicks a player from the server by handle or nucleus id", "void", "string, string");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, BanPlayerByName, "Bans a player from the server by name", "void", "string, string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, BanPlayerById, "Bans a player from the server by handle or nucleus id", "void", "string, string");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, UnbanPlayer, "Unbans a player from the server by nucleus id or ip address", "void", "string");
}
