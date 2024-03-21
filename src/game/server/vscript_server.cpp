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

#include "vscript_server.h"
#include <engine/host_state.h>
#include <networksystem/listmanager.h>
#include <random>
#include "game/server/logger.h"

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
            EServerVisibility_t eServerVisibility = static_cast<EServerVisibility_t>(sq_getinteger(v, 5));

            if (!VALID_CHARSTAR(serverName) ||
                !VALID_CHARSTAR(serverMapName) ||
                !VALID_CHARSTAR(serverPlaylist))
            {
                return SQ_OK;
            }

            // Adjust browser settings.
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);

            g_pServerListManager->m_Server.m_svHostName = serverName;
            g_pServerListManager->m_Server.m_svDescription = serverDescription;
            g_pServerListManager->m_Server.m_svHostMap = serverMapName;
            g_pServerListManager->m_Server.m_svPlaylist = serverPlaylist;
            g_pServerListManager->m_ServerVisibility = eServerVisibility;

            // Launch server.
            g_pServerListManager->LaunchServer(g_pServer->IsActive());

            return SQ_OK;

            //v_SQVM_RaiseError(v, "\"%s\" is not supported on client builds.\n", "CreateServer");
            //return SQ_ERROR;
        }
        //-----------------------------------------------------------------------------
        // Purpose: shuts the server down and disconnects all clients
        //-----------------------------------------------------------------------------
        SQRESULT DestroyServer(HSQUIRRELVM v)
        {
            if (g_pHostState->m_bActiveGame)
                g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;

            return SQ_OK;
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

            g_pBanSystem->KickPlayerByName(playerName, reason);

            return SQ_OK;
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

            g_pBanSystem->KickPlayerById(playerHandle, reason);

            return SQ_OK;
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

            g_pBanSystem->BanPlayerByName(playerName, reason);

            return SQ_OK;
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

            g_pBanSystem->BanPlayerById(playerHandle, reason);

            return SQ_OK;
        }

          //-----------------------------------------------------------------------------
          // Purpose: adds an id to banlist
          //-----------------------------------------------------------------------------
            SQRESULT AddBanByID(HSQUIRRELVM v)
            {
                SQChar* ip = sq_getstring(v, 1);
                SQChar* p_id = sq_getstring(v, 2);
                bool bResult = false;

                // Discard empty strings, this will use the default message instead.
                if (!VALID_CHARSTAR(ip))
                    ip = nullptr;

                // string to NucleusID_t
                char* endPtr = nullptr;
                NucleusID_t id = strtoull(p_id, &endPtr, 10);

                if (*endPtr != '\0') 
                {
                    bResult = false;
                }

                if (g_pBanSystem->AddEntry(ip, id))
                {   
                    g_pBanSystem->SaveList();
                    bResult = true;
                }


                sq_pushbool(v, bResult);
                return SQ_OK;

            }


        //-----------------------------------------------------------------------------
        // Purpose: unbans a player by given nucleus id or ip address
        //-----------------------------------------------------------------------------
        SQRESULT UnbanPlayer(HSQUIRRELVM v)
        {
            SQChar* szCriteria = sq_getstring(v, 1);
            g_pBanSystem->UnbanPlayer(szCriteria);

            return SQ_OK;
        }

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
        // Purpose: checks whether the server is active
        //-----------------------------------------------------------------------------
        SQRESULT IsServerActive(HSQUIRRELVM v)
        {
            bool isActive = g_pServer->IsActive();

            sq_pushbool(v, isActive);
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
    Script_RegisterCommonAbstractions(s);
    Script_RegisterCoreServerFunctions(s);
    Script_RegisterAdminPanelFunctions(s);
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

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, AddBanByID, "Adds a player to banlist by ip & nucleus id, returns true for success", "bool", "string, string");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, UnbanPlayer, "Unbans a player from the server by nucleus id or ip address", "void", "string");
}
