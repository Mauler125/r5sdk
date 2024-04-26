//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// Create functions here under the target VM namespace. If the function has to
// be registered for 2 or more VM's, put them under the 'SHARED' namespace. 
// Ifdef them out for 'SERVER_DLL' / 'CLIENT_DLL' if the target VM's do not 
// include 'SERVER' / 'CLIENT'.
//
//=============================================================================//

#include "core/stdafx.h"
#include "rtech/playlists/playlists.h"
#include "engine/client/cl_main.h"
#include "engine/cmodel_bsp.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript_shared.h"
#include "game/server/logger.h"
#pragma once
#include "game/server/gameinterface.h"
#include "common/callback.h"
#include "common/netmessages.h"
#include <networksystem/hostmanager.h>
#include <random>
#include "game/server/player.cpp"

#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // !CLIENT_DLL


namespace VScriptCode
{
    namespace Shared
    {
        //-----------------------------------------------------------------------------
        // Purpose: expose SDK version to the VScript API
        //-----------------------------------------------------------------------------
        SQRESULT GetSDKVersion(HSQUIRRELVM v)
        {
            sq_pushstring(v, SDK_VERSION, -1);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_InstalledMapsMutex);

            if (g_InstalledMaps.IsEmpty())
                SCRIPT_CHECK_AND_RETURN(v, SQ_OK);

            sq_newarray(v, 0);

            FOR_EACH_VEC(g_InstalledMaps, i)
            {
                const CUtlString& mapName = g_InstalledMaps[i];

                sq_pushstring(v, mapName.String(), -1);
                sq_arrayappend(v, -2);
            }

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available playlists
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailablePlaylists(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_PlaylistsVecMutex);

            if (g_vAllPlaylists.empty())
                SCRIPT_CHECK_AND_RETURN(v, SQ_OK);

            sq_newarray(v, 0);
            for (const string& it : g_vAllPlaylists)
            {
                sq_pushstring(v, it.c_str(), -1);
                sq_arrayappend(v, -2);
            }

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        SQRESULT ScriptError(HSQUIRRELVM v)
        {
            SQChar* pString = NULL;
            SQInteger a4 = 0;

            if (SQVM_sprintf(v, 0, 1, &a4, &pString) < 0)
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);

            v_SQVM_ScriptError("%s", pString);
            SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
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
        SQRESULT Shared::SQMatchID(HSQUIRRELVM v)
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

            if ( SQ_SUCCEEDED(sq_getinteger(v, 2, &flag)) )
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


        SQRESULT Shared::LoadKDString(HSQUIRRELVM v)
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


        SQRESULT Shared::LoadBatchKDStrings(HSQUIRRELVM v)
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


        SQRESULT Shared::GetKDString(HSQUIRRELVM v)
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


        SQRESULT Shared::SQ_UpdateLiveStats(HSQUIRRELVM v)
        {
            const SQChar* stats_json = nullptr;
         
            if (SQ_FAILED(sq_getstring(v, 2, &stats_json)))
            {
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            LOGGER::UpdateLiveStats(stats_json);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }



        SQRESULT Shared::SQ_ResetStats(HSQUIRRELVM v)
        {
            const SQChar* player_oid = nullptr;
            if (SQ_SUCCEEDED(sq_getstring(v, 2, &player_oid)) && player_oid)
            {
                LOGGER::TaskManager::getInstance().ResetPlayerStats(player_oid);
            }
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT Shared::FetchGlobalSettingsFromR5RDEV(HSQUIRRELVM v)
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

        SQRESULT Shared::SQ_GetSetting(HSQUIRRELVM v)
        {
            const SQChar* setting_key = nullptr;
            if (SQ_SUCCEEDED(sq_getstring(v, 2, &setting_key)) && setting_key)
            {
                const char* setting_value = LOGGER::GetSetting(setting_key);
                sq_pushstring(v, setting_value, -1);
            }
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }


        SQRESULT Shared::SQ_ReloadConfig(HSQUIRRELVM v)
        {
            LOGGER::ReloadConfig("r5rdev_config.json");
            SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
        }

        SQRESULT Shared::SQ_ServerMsg(HSQUIRRELVM v)
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



        SQRESULT Shared::SQ_CreateServerBot(HSQUIRRELVM v)
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
// Purpose: common script abstractions
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCommonAbstractions(CSquirrelVM* s)
{
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetSDKVersion, "Gets the SDK version as a string", "string", "");

    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetAvailableMaps, "Gets an array of all available maps", "array< string >", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetAvailablePlaylists, "Gets an array of all available playlists", "array< string >", "");

    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, ScriptError, "", "void", "string format, ...")



    //for stat settings (api keys, discord webhooks, server identifiers, preferences))
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_GetSetting, "Fetches value by key", "string", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_ReloadConfig, "Reloads R5R.DEV config file", "void", "");

    //for logging 
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, InitializeLogThread_internal, "Initializes internal logevent thread", "void", "bool");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, LogEvent, "Logs event with GameEvent,Encryption", "void", "string, bool");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQMatchID, "Gets the match ID", "string", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, stopLogging, "Stops the logging thread, writes remaining queued messages", "void", "bool");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, isLogging, "Checks if the log thread is running, atomic", "bool", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_GetLogState, "Checks various states, returns true false", "bool", "int");

    // for debugging the sqvm
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, CleanupLogs, "Deletes oldest logs in platform/eventlogs when directory exceeds 20mb", "void", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, sqprint, "Prints string to console window from sqvm", "void", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, sqerror, "Prints error string to console window from sqvm", "void", "string");

    //for verification
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, EA_Verify, "Verifys EA Account on R5R.DEV", "int", "string, string, string");

    // for stat updates
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, _STATSHOOK_UpdatePlayerCount, "Updates LIVE player count on R5R.DEV", "void", "string, string, string, string, string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, _STATSHOOK_EndOfMatch, "Updates match recap on R5R.DEV", "void", "string, string");

    //for polling stats
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_UpdateLiveStats, "Updates live server stats R5R.DEV", "void", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, LoadKDString, "Initializes grabbing stats for player", "void", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, GetKDString, "Fetches stats for player on R5R.DEV", "string", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_ResetStats, "Sets map value for player_oid stats to empty string", "void", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, LoadBatchKDStrings, "Fetches batch player stats queries", "void", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, FetchGlobalSettingsFromR5RDEV, "Fetches global settings based on query", "string", "string");

    //send a message as a bot.
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_CreateServerBot, "Creates a bot to send messages", "array< int >", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQ_ServerMsg, "Says message from specified senderId", "void", "string,int");
}

//---------------------------------------------------------------------------------
// Purpose: listen server constants
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterListenServerConstants(CSquirrelVM* s)
{
    const SQBool hasListenServer = !IsClientDLL();
    s->RegisterConstant("LISTEN_SERVER", hasListenServer);
}

//---------------------------------------------------------------------------------
// Purpose: server enums
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCommonEnums_Server(CSquirrelVM* const s)
{
    v_Script_RegisterCommonEnums_Server(s);

    if (ServerScriptRegisterEnum_Callback)
        ServerScriptRegisterEnum_Callback(s);
}

//---------------------------------------------------------------------------------
// Purpose: client/ui enums
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCommonEnums_Client(CSquirrelVM* const s)
{
    v_Script_RegisterCommonEnums_Client(s);

    const SQCONTEXT context = s->GetContext();

    if (context == SQCONTEXT::CLIENT && ClientScriptRegisterEnum_Callback)
        ClientScriptRegisterEnum_Callback(s);
    else if (context == SQCONTEXT::UI && UIScriptRegisterEnum_Callback)
        UIScriptRegisterEnum_Callback(s);
}

void VScriptShared::Detour(const bool bAttach) const
{
    DetourSetup(&v_Script_RegisterCommonEnums_Server, &Script_RegisterCommonEnums_Server, bAttach);
    DetourSetup(&v_Script_RegisterCommonEnums_Client, &Script_RegisterCommonEnums_Client, bAttach);
}
