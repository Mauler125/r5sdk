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

        //-----------------------------------------------------------------------------
        // Generate / get usable matchID 
        //-----------------------------------------------------------------------------
        std::mutex g_MatchIDMutex;
        int64_t g_MatchID = 0;

        void setMatchID(int64_t newID) {
            std::lock_guard<std::mutex> lock(g_MatchIDMutex);
            g_MatchID = newID;
        }

        int64_t getMatchID() {
            std::lock_guard<std::mutex> lock(g_MatchIDMutex);
            return g_MatchID;
        }

        SQRESULT Server::SQMatchID(HSQUIRRELVM v) {
            std::lock_guard<std::mutex> lock(g_MatchIDMutex);
            std::stringstream ss;
            ss << g_MatchID;
            std::string matchIDStr = ss.str();
            sq_pushstring(v, matchIDStr.c_str(), -1);
            return SQ_OK;
        }

        SQRESULT Server::SetMatchID(HSQUIRRELVM v) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int64_t> distr(0, INT64_MAX);

            int64_t randomNumber = distr(gen);
            auto now = std::chrono::system_clock::now();
            auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
            auto nowAsInt64 = static_cast<int64_t>(nowAsTimeT);

            int64_t newID = randomNumber + nowAsInt64;
            setMatchID(newID);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
       // Purpose: mkos funni io logger
       //-----------------------------------------------------------------------------

        SQRESULT LogEvent(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> lock(g_MatchIDMutex);
            std::stringstream ss;
            ss << Server::g_MatchID;
            std::string matchIDStr = ss.str();
           
            std::string filename = matchIDStr;
            const SQChar* logString = sq_getstring(v, 1);
            SQBool checkDir = sq_getbool(v, 2);
            SQBool encrypt = sq_getbool(v, 3);

            if (!VALID_CHARSTAR(filename.c_str()) || !VALID_CHARSTAR(logString)) {
                return SQ_ERROR;
            }

            LOGGER::LogEvent(filename.c_str(), logString, checkDir, encrypt);

            return SQ_OK;
        }

        SQRESULT stopLogging(HSQUIRRELVM v) {
            bool doSendToAPI = false;

            SQBool sendToAPI = sq_getbool(v, 1);
            if (sendToAPI) {
                doSendToAPI = sendToAPI != 0;
            }

            DevMsg(eDLL_T::SERVER, "Send to API bool set to: %s \n", doSendToAPI ? "true" : "false");
            LOGGER::stopLogging(doSendToAPI);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: mkos debug
        //-----------------------------------------------------------------------------

        SQRESULT sqprint(HSQUIRRELVM v) {

            SQChar* sqprintmsg = sq_getstring(v, 1);
            std::ostringstream oss;
            oss << sqprintmsg;

            std::string str = oss.str();
            DevMsg(eDLL_T::SERVER, ":: %s\n", str.c_str());


            return SQ_OK;
        }

        SQRESULT testbool(HSQUIRRELVM v) {
            SQBool sqbool = sq_getbool(v, 1);
            std::string returnvalue = sqbool ? "true" : "false";

            DevMsg(eDLL_T::SERVER, "Bool value: %s \n", returnvalue.c_str());

            sq_pushbool(v, sqbool);

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
    //for logging
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LogEvent, "Logs event with GameEvent,DirectoryCheck,Encryption", "void", "string, bool, bool");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SetMatchID, "Sets the match ID", "void", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQMatchID, "Gets the match ID", "string", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, stopLogging, "Stops the logging thread, writes remaining queued messages", "void", "bool");
    // for debugging the sqvm
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, sqprint, "Prints string to console window from sqvm", "string", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, testbool, "Prints string to console window from sqvm", "bool", "bool");
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
