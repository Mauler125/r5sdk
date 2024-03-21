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
#include "vpc/keyvalues.h"
#include "engine/client/cl_main.h"
#include "engine/cmodel_bsp.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript_shared.h"
#include "game/server/logger.h"
#include <random>

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
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_InstalledMapsMutex);

            if (g_InstalledMaps.IsEmpty())
                return SQ_OK;

            sq_newarray(v, 0);

            FOR_EACH_VEC(g_InstalledMaps, i)
            {
                const CUtlString& mapName = g_InstalledMaps[i];

                sq_pushstring(v, mapName.String(), -1);
                sq_arrayappend(v, -2);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available playlists
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailablePlaylists(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_PlaylistsVecMutex);

            if (g_vAllPlaylists.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (const string& it : g_vAllPlaylists)
            {
                sq_pushstring(v, it.c_str(), -1);
                sq_arrayappend(v, -2);
            }

            return SQ_OK;
        }

        SQRESULT ScriptError(HSQUIRRELVM v)
        {
            SQChar* pString = NULL;
            SQInteger a4 = 0;

            if (SQVM_sprintf(v, 0, 1, &a4, &pString) < 0)
                return SQ_ERROR;

            v_SQVM_ScriptError("%s", pString);

            // this should be moved to a wrapper for all script funcs
            if (*reinterpret_cast<DWORD*>(&v->_sharedstate->gap43b9[127]))
            {
                v_SQVM_ThrowError(*reinterpret_cast<QWORD*>(&v->_sharedstate->gap43b9[111]), v);
            }

            return SQ_ERROR;
        }

        //-----------------------------------------------------------------------------
        // Generate / get usable matchID 
        //-----------------------------------------------------------------------------

        std::atomic<int64_t> g_MatchID{0};

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
            return SQ_OK;
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

        // Check of is currently running -- returns true if not logging, false if running
        SQRESULT isLogging(HSQUIRRELVM v) 
        {
            bool state = LOGGER::Logger::getInstance().isLogging();
            sq_pushbool(v, state);
            return SQ_OK;
        }

        SQRESULT SQ_GetLogState(HSQUIRRELVM v)
        {  
            if ((sq_getinteger(v, 1) >= 0))
            {
                SQInteger flag = sq_getinteger(v, 1);
                LOGGER::Logger& logger = LOGGER::Logger::getInstance();
                LOGGER::Logger::LogState LogState = logger.intToLogState(flag);
                bool state = logger.getLogState(LogState);
                sq_pushbool(v, state);
            }
            else
            {
                Error(eDLL_T::SERVER, NO_ERROR, "SQ_ERROR: sq_getinteger(v, 1) value: %s", std::to_string(sq_getinteger(v, 1)).c_str());
                sq_pushbool(v, false); 
            }
            return SQ_OK;
        }

        

        SQRESULT LogEvent(HSQUIRRELVM v)
        {
            const SQChar* logString = sq_getstring(v, 1);
            SQBool encrypt = sq_getbool(v, 2);

            if (!VALID_CHARSTAR(logString)) 
            {
                Error(eDLL_T::SERVER, NO_ERROR, "INVALID CHARSTAR");
                return SQ_ERROR;
            }
            
           
            LOGGER::pMkosLogger->LogEvent(logString, encrypt);

            return SQ_OK;
        }

        SQRESULT InitializeLogThread_internal(HSQUIRRELVM v)
        {   
            if (getMatchID() == 0)
            {
                selfSetMatchID();
            }

            SQBool encrypt = sq_getbool(v, 1);
            SQBool state = true;

            LOGGER::Logger& logger = LOGGER::Logger::getInstance();
            logger.InitializeLogThread(encrypt);

            sq_pushbool(v, state);

            return SQ_OK;
        }


        SQRESULT stopLogging(HSQUIRRELVM v) 
        {

            bool doSendToAPI = false;

            SQBool sendToAPI = sq_getbool(v, 1);

            if (sendToAPI) 
            {
                doSendToAPI = sendToAPI != 0;
            }

            DevMsg(eDLL_T::SERVER, "Send to API bool set to: %s \n", doSendToAPI ? "true" : "false");
            LOGGER::pMkosLogger->stopLogging(doSendToAPI);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: deletes oldest logs after specified MB limit within specified 
        //          logfolder defined in settings json
        //-----------------------------------------------------------------------------

        SQRESULT CleanupLogs(HSQUIRRELVM v)
        {
            LOGGER::CleanupLogs(FileSystem());
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: mkos debug - prints to console without devmode
        //-----------------------------------------------------------------------------

        SQRESULT sqprint(HSQUIRRELVM v) 
        {
            SQChar* sqprintmsg = sq_getstring(v, 1);
            std::ostringstream oss;
            oss << sqprintmsg;

            std::string str = oss.str();
            DevMsg(eDLL_T::SERVER, ":: %s\n", str.c_str());

            return SQ_OK;
        }

        SQRESULT sqerror(HSQUIRRELVM v)
        {
            SQChar* sqprintmsg = sq_getstring(v, 1);
            std::ostringstream oss;
            oss << sqprintmsg;

            std::string str = oss.str();
            Error(eDLL_T::SERVER, NO_ERROR, ":: %s\n", str.c_str());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: None whatsoever
        //-----------------------------------------------------------------------------

        SQRESULT testbool(HSQUIRRELVM v) 
        {
            SQBool sqbool = sq_getbool(v, 1);
            std::string returnvalue = sqbool ? "true" : "false";

            DevMsg(eDLL_T::SERVER, "Bool value: %s \n", returnvalue.c_str());

            sq_pushbool(v, sqbool);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: facilitates communication between sqvm and logger api calls 
        // for ea account verification
        //-----------------------------------------------------------------------------

        SQRESULT EA_Verify(HSQUIRRELVM v) 
        {
            const SQChar* token = sq_getstring(v, 1);
            const SQChar* OID = sq_getstring(v, 2);
            const SQChar* ea_name = sq_getstring(v, 3);

            int32_t status_num = 0;
            string status = LOGGER::VERIFY_EA_ACCOUNT(token, OID, ea_name);
            try {
                status_num = std::stoi(status);
                //DevMsg(eDLL_T::SERVER, "Conversion successful, number is: %d\n", status_num);
            }
            catch (const std::invalid_argument& e) {
                DevMsg(eDLL_T::SERVER, "Error: Invalid argument for conversion: %s\n", e.what());
            }
            catch (const std::out_of_range& e) {
                DevMsg(eDLL_T::SERVER, "Error: Value out of range for conversion: %s\n", e.what());
            }

            sq_pushinteger(v, status_num);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: api calls for stats
        //-----------------------------------------------------------------------------

        SQRESULT _STATSHOOK_UpdatePlayerCount(HSQUIRRELVM v) 
        {
            const SQChar* action = sq_getstring(v, 1);
            const SQChar* player = sq_getstring(v, 2);
            const SQChar* OID = sq_getstring(v, 3);
            const SQChar* count = sq_getstring(v, 4);
            const SQChar* DISCORD_HOOK = sq_getstring(v, 5);

            LOGGER::UPDATE_PLAYER_COUNT(action, player, OID, count, DISCORD_HOOK);

            return SQ_OK;
        }

        SQRESULT _STATSHOOK_EndOfMatch(HSQUIRRELVM v) 
        {      
            const SQChar* recap = sq_getstring(v, 1);
            const SQChar* DISCORD_HOOK = sq_getstring(v, 2);

            LOGGER::NOTIFY_END_OF_MATCH( recap, DISCORD_HOOK );
            return SQ_OK;
        }

        SQRESULT Shared::LoadKDString(HSQUIRRELVM v) 
        {
            const SQChar* player_oid = sq_getstring(v, 1);

            LOGGER::TaskManager::getInstance().LoadKDString(player_oid);
            
            return SQ_OK;
        }

        SQRESULT Shared::LoadBatchKDStrings(HSQUIRRELVM v) 
        {
            const SQChar* player_oids = sq_getstring(v, 1);

            LOGGER::TaskManager::getInstance().LoadBatchKDStrings(player_oids);
         
            return SQ_OK;
        }

        SQRESULT Shared::GetKDString(HSQUIRRELVM v) 
        {
            const SQChar* player_oid = sq_getstring(v, 1);
            std::string stats = LOGGER::GetKDString(player_oid);

            sq_pushstring(v, stats.c_str(), -1);

            return SQ_OK;
        }


        SQRESULT Shared::SQ_UpdateLiveStats(HSQUIRRELVM v) 
        {
            std::string stats_json = sq_getstring(v, 1);
            LOGGER::UpdateLiveStats(stats_json);

            return SQ_OK;
        }

        SQRESULT Shared::SQ_ResetStats(HSQUIRRELVM v) 
        {
            const SQChar* player_oid = sq_getstring(v, 1);

            LOGGER::TaskManager::getInstance().ResetPlayerStats(player_oid);

            return SQ_OK;
        }

        SQRESULT Shared::FetchGlobalSettingsFromR5RDEV(HSQUIRRELVM v)
        {
            const SQChar* query = sq_getstring(v, 1);
            std::string settings = LOGGER::FetchGlobalSettings(query);

            sq_pushstring(v, settings.c_str(), -1);

            return SQ_OK;
        }


        //-----------------------------------------------------------------------------
        // Purpose: fetches a setting value by key in the settings map. 
        // loaded from (r5rdev_config)
        //-----------------------------------------------------------------------------

        SQRESULT Shared::SQ_GetSetting(HSQUIRRELVM v)
        {
            const SQChar* setting_key = sq_getstring(v, 1);
            const char* setting_value = LOGGER::GetSetting(setting_key);

            sq_pushstring(v, setting_value, -1);

            return SQ_OK;
        }

        SQRESULT Shared::SQ_ReloadConfig(HSQUIRRELVM v)
        {
            LOGGER::ReloadConfig("r5rdev_config.json");
            return SQ_OK;
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
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, testbool, "Prints string to console window from sqvm", "bool", "bool");

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
  
}

//---------------------------------------------------------------------------------
// Purpose: listen server constants (!!! only call on builds containing a listen server !!!)
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterListenServerConstants(CSquirrelVM* s)
{
    const SQBool hasListenServer = !IsClientDLL();
    s->RegisterConstant("LISTEN_SERVER", hasListenServer);
}
