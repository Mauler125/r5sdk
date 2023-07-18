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
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // CLIENT_DLL
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#include "networksystem/pylon.h"
#ifndef CLIENT_DLL
#include "networksystem/bansystem.h"
#endif // !CLIENT_DLL
#ifndef SERVER_DLL
#include "networksystem/listmanager.h"
#endif // !SERVER_DLL
#include "vscript_shared.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

namespace VScriptCode
{
    namespace Shared
    {
        //-----------------------------------------------------------------------------
        // Purpose: SDK test and example body
        //-----------------------------------------------------------------------------
        SQRESULT SDKNativeTest(HSQUIRRELVM v)
        {
            // Function code goes here.
            return SQ_OK;
        }

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

            if (g_InstalledMaps.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (const string& it : g_InstalledMaps)
            {
                sq_pushstring(v, it.c_str(), -1);
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
        //-----------------------------------------------------------------------------
        // Purpose: checks whether the server is active
        //-----------------------------------------------------------------------------
        SQRESULT IsServerActive(HSQUIRRELVM v)
        {
            bool isActive = false;
#ifndef CLIENT_DLL
            isActive = g_pServer->IsActive();
#endif // !CLIENT_DLL

            sq_pushbool(v, isActive);
            return SQ_OK;
        }
#ifndef CLIENT_DLL

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
#endif // !CLIENT_DLL
        //-----------------------------------------------------------------------------
        // Purpose: shutdown local game (host only)
        //-----------------------------------------------------------------------------
        SQRESULT ShutdownHostGame(HSQUIRRELVM v)
        {
            if (g_pHostState->m_bActiveGame)
                g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: checks whether this SDK build is a client dll
        //-----------------------------------------------------------------------------
        SQRESULT IsClientDLL(HSQUIRRELVM v)
        {
#ifdef CLIENT_DLL
            constexpr SQBool bClientOnly = true;
#else
            constexpr SQBool bClientOnly = false;
#endif
            sq_pushbool(v, bClientOnly);
            return SQ_OK;
        }
    }
}
