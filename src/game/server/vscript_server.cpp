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

#include "game/server/liveapi/liveapi.h"
#include "vscript_server.h"
#include <engine/host_state.h>
#include <networksystem/listmanager.h>
#include <random>

#pragma once
#include "game/server/player.cpp"

#ifndef CLIENT_DLL
    #include "game/server/logger.h"
#endif

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
static ConCommand script("script", SQVM_ServerScript_f, "Run input code as SERVER script on the VM", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT | FCVAR_SERVER_FRAME_THREAD);

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
            const SQChar* serverName = nullptr;
            const SQChar* serverDescription = nullptr;
            const SQChar* serverMapName = nullptr;
            const SQChar* serverPlaylist = nullptr;

            sq_getstring(v, 2, &serverName);
            sq_getstring(v, 3, &serverDescription);
            sq_getstring(v, 4, &serverMapName);
            sq_getstring(v, 5, &serverPlaylist);

            SQInteger serverVisibility = 0;
            sq_getinteger(v, 6, &serverVisibility);

            if (!VALID_CHARSTAR(serverName) ||
                !VALID_CHARSTAR(serverMapName) ||
                !VALID_CHARSTAR(serverPlaylist))
            {
                v_SQVM_ScriptError("Empty or null server criteria");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            // Adjust browser settings.
            NetGameServer_t& details = g_ServerHostManager.GetDetails();

            details.name = serverName;
            details.description = serverDescription;
            details.map = serverMapName;
            details.playlist = serverPlaylist;

            // Launch server.
            g_ServerHostManager.SetVisibility(ServerVisibility_e(serverVisibility));
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
        // Purpose: sets whether the server could auto reload at this time (e.g. if
        // server admin has host_autoReloadRate AND host_autoReloadRespectGameState
        // set, and its time to auto reload, but the match hasn't finished yet, wait
        // until this is set to proceed the reload of the server
        //-----------------------------------------------------------------------------
        SQRESULT SetAutoReloadState(HSQUIRRELVM v)
        {
            SQBool state = false;
            sq_getbool(v, 2, &state);

            g_hostReloadState = state;
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerByName(HSQUIRRELVM v)
        {
            const SQChar* playerName = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerName);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerName))
            {
                v_SQVM_ScriptError("Empty or null player name");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

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
            const SQChar* playerHandle = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerHandle);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerHandle))
            {
                v_SQVM_ScriptError("Empty or null player handle");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

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
            const SQChar* playerName = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerName);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerName))
            {
                v_SQVM_ScriptError("Empty or null player name");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

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
            const SQChar* playerHandle = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerHandle);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerHandle))
            {
                v_SQVM_ScriptError("Empty or null player handle");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.BanPlayerById(playerHandle, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

          //-----------------------------------------------------------------------------
          // Purpose: adds an id to banlist
          //-----------------------------------------------------------------------------
            SQRESULT AddBanByID(HSQUIRRELVM v)
            {
                const SQChar* ip = nullptr;
                const SQChar* p_id = nullptr;

                sq_getstring(v, 2, &ip );
                sq_getstring(v, 3, &p_id );

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

                if (g_BanSystem.AddEntry(ip, id))
                {   
                    g_BanSystem.SaveList();
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
            const SQChar* szCriteria = nullptr;
            sq_getstring(v, 2, &szCriteria);

            if (!VALID_CHARSTAR(szCriteria))
            {
                v_SQVM_ScriptError("Empty or null player criteria");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

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
        // Purpose: gets the current server id
        //-----------------------------------------------------------------------------
        SQRESULT GetServerID(HSQUIRRELVM v)
        {
            sq_pushstring(v, g_LogSessionUUID.c_str(), (SQInteger)g_LogSessionUUID.length());
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

        //-----------------------------------------------------------------------------
        // Generate / get usable matchID 
        //-----------------------------------------------------------------------------

        std::atomic<int64_t> g_MatchID{ 0 };

        //not exposed to sqvm
        void setMatchID(int64_t newID)
        {
            g_MatchID.store(newID);
        }

        //not exposed to sqvm
        int64_t getMatchID()
        {
            return g_MatchID.load();
        }


        // exposed to sqvm - retrieves matchID
        SQRESULT SQMatchID(HSQUIRRELVM v)
        {
            std::string matchIDStr = std::to_string(getMatchID());
            sq_pushstring(v, matchIDStr.c_str(), -1);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        // set match ID
        int64_t selfSetMatchID()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int64_t> distr(0, INT64_MAX);

            int64_t randomNumber = distr(gen);
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t nowAsTimeT = std::chrono::system_clock::to_time_t(now);
            int64_t nowAsInt64 = static_cast<int64_t>(nowAsTimeT);

            int64_t newID = randomNumber + nowAsInt64;
            setMatchID(newID);

            return newID;
        }


        //-----------------------------------------------------------------------------
        // Purpose: mkos funni io logger
        //-----------------------------------------------------------------------------

        // Check of is currently running -- returns true if logging, false if not running
        SQRESULT isLogging(HSQUIRRELVM v)
        {
            bool state = LOGGER::Logger::getInstance().isLogging();
            sq_pushbool(v, state);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        SQRESULT SQ_GetLogState(HSQUIRRELVM v)
        {
            SQInteger flag = NULL;

            if (SQ_SUCCEEDED(sq_getinteger(v, 2, &flag)))
            {
                LOGGER::Logger& logger = LOGGER::Logger::getInstance();
                LOGGER::Logger::LogState LogState = logger.intToLogState(flag);
                bool state = logger.getLogState(LogState);
                sq_pushbool(v, state);
                SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
            }
            else
            {
                Error(eDLL_T::SERVER, NO_ERROR, "SQ_ERROR: SQ_GetLogState");
                sq_pushbool(v, false);
            }

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }



        SQRESULT LogEvent(HSQUIRRELVM v)
        {
            const SQChar* logString = nullptr;
            SQBool encrypt = false;

            SQRESULT getStringResult = sq_getstring(v, 2, &logString);
            SQRESULT getBoolResult = sq_getbool(v, 3, &encrypt);

            if (SQ_SUCCEEDED(getStringResult) && SQ_SUCCEEDED(getBoolResult))
            {
                if (!VALID_CHARSTAR(logString))
                {
                    Error(eDLL_T::SERVER, NO_ERROR, "INVALID CHARSTAR");
                    SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
                }

                LOGGER::pMkosLogger->LogEvent(logString, encrypt);
                return SQ_OK;
            }
            else
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Error retrieving parameters.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }
        }


        SQRESULT InitializeLogThread_internal(HSQUIRRELVM v)
        {
            SQBool encrypt = false;
            if (getMatchID() == 0)
            {
                selfSetMatchID();
            }

            if (SQ_FAILED(sq_getbool(v, 2, &encrypt)))
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'encrypt' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::Logger& logger = LOGGER::Logger::getInstance();
            logger.InitializeLogThread(encrypt);

            sq_pushbool(v, true);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }



        SQRESULT stopLogging(HSQUIRRELVM v)
        {
            SQBool sendToAPI = false;

            if (SQ_FAILED(sq_getbool(v, 2, &sendToAPI)))
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'sendToAPI' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            bool doSendToAPI = sendToAPI != 0;

            //DevMsg(eDLL_T::SERVER, "Send to API bool set to: %s \n", doSendToAPI ? "true" : "false");
            LOGGER::pMkosLogger->stopLogging(doSendToAPI);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: deletes oldest logs after specified MB limit within specified 
        //          logfolder defined in settings json
        //-----------------------------------------------------------------------------

        SQRESULT CleanupLogs(HSQUIRRELVM v)
        {
            LOGGER::CleanupLogs(FileSystem());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: mkos debug - prints to console without devmode
        //-----------------------------------------------------------------------------

        SQRESULT sqprint(HSQUIRRELVM v)
        {
            const SQChar* sqprintmsg = nullptr;
            SQRESULT res = sq_getstring(v, 2, &sqprintmsg);

            if (SQ_FAILED(res) || !sqprintmsg)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'sqprintmsg' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            std::string str = sqprintmsg;
            Msg(eDLL_T::SERVER, ":: %s\n", str.c_str());

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT sqerror(HSQUIRRELVM v)
        {
            const SQChar* sqprintmsg = nullptr;
            SQRESULT res = sq_getstring(v, 2, &sqprintmsg);

            if (SQ_FAILED(res) || !sqprintmsg)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'sqprintmsg' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            std::string str = sqprintmsg;
            Error(eDLL_T::SERVER, NO_ERROR, ":: %s\n", str.c_str());

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: facilitates communication between sqvm and logger api calls 
        // for ea account verification
        //-----------------------------------------------------------------------------

        SQRESULT EA_Verify(HSQUIRRELVM v)
        {
            const SQChar* token = nullptr;
            const SQChar* OID = nullptr;
            const SQChar* ea_name = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &token)) || !token ||
                SQ_FAILED(sq_getstring(v, 3, &OID)) || !OID ||
                SQ_FAILED(sq_getstring(v, 4, &ea_name)) || !ea_name)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve parameters.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            int32_t status_num = 0;
            string status = LOGGER::VERIFY_EA_ACCOUNT(token, OID, ea_name);

            try {
                status_num = std::stoi(status);
            }
            catch (const std::invalid_argument& e) {
                Msg(eDLL_T::SERVER, "Error: Invalid argument for conversion: %s\n", e.what());
            }
            catch (const std::out_of_range& e) {
                Msg(eDLL_T::SERVER, "Error: Value out of range for conversion: %s\n", e.what());
            }

            sq_pushinteger(v, status_num);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        //-----------------------------------------------------------------------------
        // Purpose: api calls for stats
        //-----------------------------------------------------------------------------

        SQRESULT _STATSHOOK_UpdatePlayerCount(HSQUIRRELVM v)
        {
            const SQChar* action = nullptr;
            const SQChar* player = nullptr;
            const SQChar* OID = nullptr;
            const SQChar* count = nullptr;
            const SQChar* DISCORD_HOOK = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &action)) || !action ||
                SQ_FAILED(sq_getstring(v, 3, &player)) || !player ||
                SQ_FAILED(sq_getstring(v, 4, &OID)) || !OID ||
                SQ_FAILED(sq_getstring(v, 5, &count)) || !count ||
                SQ_FAILED(sq_getstring(v, 6, &DISCORD_HOOK)) || !DISCORD_HOOK)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve parameters.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::UPDATE_PLAYER_COUNT(action, player, OID, count, DISCORD_HOOK);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT _STATSHOOK_EndOfMatch(HSQUIRRELVM v)
        {
            const SQChar* recap = nullptr;
            const SQChar* DISCORD_HOOK = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &recap)) || !recap ||
                SQ_FAILED(sq_getstring(v, 3, &DISCORD_HOOK)) || !DISCORD_HOOK)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve parameters.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::NOTIFY_END_OF_MATCH(recap, DISCORD_HOOK);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT LoadKDString(HSQUIRRELVM v)
        {
            const SQChar* player_oid = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &player_oid)) || !player_oid)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'player_oid' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::TaskManager::getInstance().LoadKDString(player_oid);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT LoadBatchKDStrings(HSQUIRRELVM v)
        {
            const SQChar* player_oids = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &player_oids)) || !player_oids)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'player_oids' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::TaskManager::getInstance().LoadBatchKDStrings(player_oids);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT GetKDString(HSQUIRRELVM v)
        {
            const SQChar* player_oid = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &player_oid)) || !player_oid)
            {
                Error(eDLL_T::SERVER, NO_ERROR, "Failed to retrieve 'player_oid' parameter.");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            std::string stats = LOGGER::GetKDString(player_oid);
            sq_pushstring(v, stats.c_str(), -1);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT SQ_UpdateLiveStats(HSQUIRRELVM v)
        {
            const SQChar* stats_json = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &stats_json)))
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::UpdateLiveStats(stats_json);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }



        SQRESULT SQ_ResetStats(HSQUIRRELVM v)
        {
            const SQChar* player_oid = nullptr;
            if (SQ_SUCCEEDED(sq_getstring(v, 2, &player_oid)) && player_oid)
            {
                LOGGER::TaskManager::getInstance().ResetPlayerStats(player_oid);
            }
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT FetchGlobalSettingsFromR5RDEV(HSQUIRRELVM v)
        {
            const SQChar* query = nullptr;
            if (SQ_SUCCEEDED(sq_getstring(v, 2, &query)) && query)
            {
                std::string settings = LOGGER::FetchGlobalSettings(query);
                sq_pushstring(v, settings.c_str(), -1);
            }
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }



        //-----------------------------------------------------------------------------
        // Purpose: fetches a setting value by key in the settings map. 
        // loaded from (r5rdev_config)
        //-----------------------------------------------------------------------------

        SQRESULT SQ_GetSetting(HSQUIRRELVM v)
        {
            const SQChar* setting_key = nullptr;
            if (SQ_SUCCEEDED(sq_getstring(v, 2, &setting_key)) && setting_key)
            {
                const char* setting_value = LOGGER::GetSetting(setting_key);
                sq_pushstring(v, setting_value, -1);
            }
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT SQ_ReloadConfig(HSQUIRRELVM v)
        {
            LOGGER::ReloadConfig("r5rdev_config.json");
            SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
        }

        SQRESULT SQ_ServerMsg(HSQUIRRELVM v)
        {
            const SQChar* inMsg = nullptr;

            if (SQ_FAILED(sq_getstring(v, 2, &inMsg)) || !inMsg)
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            SQInteger senderId = 0;

            if (SQ_FAILED(sq_getinteger(v, 3, &senderId)) || senderId < 0 || senderId > 255)
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            const std::string msg(inMsg);
            void* thisptr = nullptr;

            CServerGameDLL::OnReceivedSayTextMessage(thisptr, static_cast<int>(senderId), msg.c_str(), false);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }



        SQRESULT SQ_CreateServerBot(HSQUIRRELVM v)
        {
            if (!g_pServer->IsActive())
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            const SQChar* ImmutableName = nullptr;
            if (SQ_FAILED(sq_getstring(v, 2, &ImmutableName)) || !ImmutableName)
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            std::vector<std::string> args = { "sv_addbot", "[" + std::string(ImmutableName) + "]", "1" };

            const char* c_args[3];

            for (size_t i = 0; i < args.size(); ++i)
            {
                c_args[i] = args[i].c_str();
            }

            CCommand cmd(3, c_args, cmd_source_t::kCommandSrcUserInput);
            CC_CreateFakePlayer_f(cmd);

            for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
            {
                CClient* pClient = g_pServer->GetClient(i);

                if (!pClient)
                {
                    continue;
                }

                if (!pClient->IsHumanPlayer())
                {
                    const CNetChan* pNetChan = pClient->GetNetChan();

                    if (pNetChan->GetName() == "[" + std::string(ImmutableName) + "]")
                    {
                        int ID = pClient->GetUserID();

                        if (ID >= 0 && ID < 119)
                        {
                            sq_newarray(v, 0);
                            sq_pushinteger(v, static_cast<int>(pClient->GetHandle()));
                            sq_arrayappend(v, -2);
                            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
                        }
                    }
                }
            }

            //return array with -1 at element[0]
            sq_newarray(v, 0);
            sq_pushinteger(v, -1);
            sq_arrayappend(v, -2);
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


    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SetAutoReloadState, "Set whether we can auto-reload the server", "void", "bool");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetServerID, "Gets the current server ID", "string", "");
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


    //for stat settings (api keys, discord webhooks, server identifiers, preferences))
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_GetSetting, "Fetches value by key", "string", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_ReloadConfig, "Reloads R5R.DEV config file", "void", "");

    //for logging 
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, InitializeLogThread_internal, "Initializes internal logevent thread", "void", "bool");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LogEvent, "Logs event with GameEvent,Encryption", "void", "string, bool");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQMatchID, "Gets the match ID", "string", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, stopLogging, "Stops the logging thread, writes remaining queued messages", "void", "bool");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, isLogging, "Checks if the log thread is running, atomic", "bool", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_GetLogState, "Checks various states, returns true false", "bool", "int");

    // for debugging the sqvm
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, CleanupLogs, "Deletes oldest logs in platform/eventlogs when directory exceeds 20mb", "void", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, sqprint, "Prints string to console window from sqvm", "void", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, sqerror, "Prints error string to console window from sqvm", "void", "string");

    //for verification
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, EA_Verify, "Verifys EA Account on R5R.DEV", "int", "string, string, string");

    // for stat updates
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, _STATSHOOK_UpdatePlayerCount, "Updates LIVE player count on R5R.DEV", "void", "string, string, string, string, string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, _STATSHOOK_EndOfMatch, "Updates match recap on R5R.DEV", "void", "string, string");

    //for polling stats
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_UpdateLiveStats, "Updates live server stats R5R.DEV", "void", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LoadKDString, "Initializes grabbing stats for player", "void", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetKDString, "Fetches stats for player on R5R.DEV", "string", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_ResetStats, "Sets map value for player_oid stats to empty string", "void", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LoadBatchKDStrings, "Fetches batch player stats queries", "void", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, FetchGlobalSettingsFromR5RDEV, "Fetches global settings based on query", "string", "string");

    //send a message as a bot.
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_CreateServerBot, "Creates a bot to send messages", "array< int >", "string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SQ_ServerMsg, "Says message from specified senderId", "void", "string,int");
}
