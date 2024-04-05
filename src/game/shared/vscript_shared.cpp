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
