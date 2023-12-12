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
        void setMatchID(int64_t newID) {
            g_MatchID = newID;
        }

        //not exposed to sqvm
        int64_t VScriptCode::Shared::getMatchID() {
            return g_MatchID;
        }


        // exposed to sqvm - retrieves matchID
        SQRESULT Shared::SQMatchID(HSQUIRRELVM v) {
            std::string matchIDStr = std::to_string(g_MatchID);
            sq_pushstring(v, matchIDStr.c_str(), -1);
            return SQ_OK;
        }

        // exposed to sqvm - Sets new matchid from scripts
        SQRESULT Shared::SetMatchID(HSQUIRRELVM v) {
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


        // Give control of logger to set match ID if none was set in scripts. 
        int64_t selfSetMatchID() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int64_t> distr(0, INT64_MAX);

            int64_t randomNumber = distr(gen);
            auto now = std::chrono::system_clock::now();
            auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
            auto nowAsInt64 = static_cast<int64_t>(nowAsTimeT);

            int64_t newID = randomNumber + nowAsInt64;
            setMatchID(newID);

            return newID;
        }


        //-----------------------------------------------------------------------------
       // Purpose: mkos funni io logger
       //-----------------------------------------------------------------------------


        // Check is currently running -- returns true if logging, false if not running
        SQRESULT isLogging(HSQUIRRELVM v) {

            bool isRunning = LOGGER::Logger::getInstance().isLogging();
            sq_pushbool(v, isRunning);
            return SQ_OK;
        }


        SQRESULT LogEvent(HSQUIRRELVM v)
        {

            if (getMatchID() == 0)
            {
                selfSetMatchID();
            }
            std::string matchIDstr = std::to_string(g_MatchID);

            std::string filename = matchIDstr;
            const SQChar* logString = sq_getstring(v, 1);
            SQBool checkDir = sq_getbool(v, 2);
            SQBool encrypt = sq_getbool(v, 3);


            if (!VALID_CHARSTAR(filename.c_str()) || !VALID_CHARSTAR(logString)) {
                return SQ_ERROR;
            }


            LOGGER::Logger& logger = LOGGER::Logger::getInstance();
            logger.LogEvent(filename.c_str(), logString, checkDir, encrypt);

            return SQ_OK;
        }


        SQRESULT stopLogging(HSQUIRRELVM v) {
            bool doSendToAPI = false;

            SQBool sendToAPI = sq_getbool(v, 1);
            if (sendToAPI) {
                doSendToAPI = sendToAPI != 0;
            }

            DevMsg(eDLL_T::SERVER, "Send to API bool set to: %s \n", doSendToAPI ? "true" : "false");
            LOGGER::Logger::getInstance().stopLogging(doSendToAPI);

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

        SQRESULT EA_Verify(HSQUIRRELVM v) {
            const SQChar* token = sq_getstring(v, 1);
            const SQChar* ea_name = sq_getstring(v, 2);

            int32_t status_num = 0;
            string status = LOGGER::VERIFY_EA_ACCOUNT(token,ea_name);
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

    //for logging
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, LogEvent, "Logs event with GameEvent,DirectoryCheck,Encryption", "void", "string, bool, bool");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SetMatchID, "Sets the match ID", "void", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, SQMatchID, "Gets the match ID", "string", "");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, stopLogging, "Stops the logging thread, writes remaining queued messages", "void", "bool");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, isLogging, "Checks if the log thread is running", "bool", "");

    // for debugging the sqvm
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, sqprint, "Prints string to console window from sqvm", "string", "string");
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, testbool, "Prints string to console window from sqvm", "bool", "bool");

    //for verification
    DEFINE_SHARED_SCRIPTFUNC_NAMED(s, EA_Verify, "Verifys EA Account on R5R.DEV", "int", "string, string");
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
