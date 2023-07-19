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
#include "engine/server/server.h"
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"
#include "networksystem/listmanager.h"
#include "vscript_shared.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

namespace VScriptCode
{
    namespace Shared
    {
        //-----------------------------------------------------------------------------
        // Purpose: generic stub for unsupported functions
        //-----------------------------------------------------------------------------
        SQRESULT StubUnsupported(HSQUIRRELVM v)
        {
            v_SQVM_RaiseError(v, "This function is not supported on this build\n");
            return SQ_ERROR;
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
}
