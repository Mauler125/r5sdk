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
#include "vpc/keyvalues.h"
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#include "engine/client/cl_main.h"
#include "networksystem/pylon.h"
#include "networksystem/listmanager.h"
#include "game/shared/vscript_shared.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

#include "vscript_client.h"

//-----------------------------------------------------------------------------
// Purpose: checks if the server index is valid, raises an error if not
//-----------------------------------------------------------------------------
static SQBool Script_CheckServerIndex(HSQUIRRELVM v, SQInteger iServer)
{
    SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

    if (iServer >= iCount)
    {
        v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
        return false;
    }

    return true;
}

namespace VScriptCode
{
    namespace Client
    {
        //-----------------------------------------------------------------------------
        // Purpose: refreshes the server list
        //-----------------------------------------------------------------------------
        SQRESULT RefreshServerList(HSQUIRRELVM v)
        {
            string serverMessage; // Refresh list.
            size_t iCount = g_pServerListManager->RefreshServerList(serverMessage);

            sq_pushinteger(v, static_cast<SQInteger>(iCount));

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get current server count from pylon
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCount(HSQUIRRELVM v)
        {
            size_t iCount = g_pServerListManager->m_vServerList.size();
            sq_pushinteger(v, static_cast<SQInteger>(iCount));

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get response from private server request
        //-----------------------------------------------------------------------------
        SQRESULT GetHiddenServerName(HSQUIRRELVM v)
        {
            SQChar* privateToken = sq_getstring(v, 1);

            if (!VALID_CHARSTAR(privateToken))
                return SQ_OK;

            string hiddenServerRequestMessage;
            NetGameServer_t serverListing;

            bool result = g_pMasterServer->GetServerByToken(serverListing, hiddenServerRequestMessage, privateToken); // Send token connect request.
            if (!result)
            {
                if (hiddenServerRequestMessage.empty())
                {
                    sq_pushstring(v, "Request failed", -1);
                }
                else
                {
                    hiddenServerRequestMessage = Format("Request failed: %s", hiddenServerRequestMessage.c_str());
                    sq_pushstring(v, hiddenServerRequestMessage.c_str(), -1);
                }

                return SQ_OK;
            }

            if (serverListing.m_svHostName.empty())
            {
                if (hiddenServerRequestMessage.empty())
                {
                    hiddenServerRequestMessage = Format("Server listing empty");
                }
                else
                {
                    hiddenServerRequestMessage = Format("Server listing empty: %s", hiddenServerRequestMessage.c_str());
                }

                sq_pushstring(v, hiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                hiddenServerRequestMessage = Format("Found server: %s", serverListing.m_svHostName.c_str());
                sq_pushstring(v, hiddenServerRequestMessage.c_str(), -1);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current name from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerName(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const string& serverName = g_pServerListManager->m_vServerList[iServer].m_svHostName;
            sq_pushstring(v, serverName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current description from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerDescription(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const string& serverDescription = g_pServerListManager->m_vServerList[iServer].m_svDescription;
            sq_pushstring(v, serverDescription.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current map via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMap(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const string& svServerMapName = g_pServerListManager->m_vServerList[iServer].m_svHostMap;
            sq_pushstring(v, svServerMapName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current playlist via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerPlaylist(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const string& serverPlaylist = g_pServerListManager->m_vServerList[iServer].m_svPlaylist;
            sq_pushstring(v, serverPlaylist.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current player count via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCurrentPlayers(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const SQInteger playerCount = g_pServerListManager->m_vServerList[iServer].m_nPlayerCount;
            sq_pushinteger(v, playerCount);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current player count via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMaxPlayers(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const SQInteger maxPlayers = g_pServerListManager->m_vServerList[iServer].m_nMaxPlayers;
            sq_pushinteger(v, maxPlayers);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get promo data for serverbrowser panels
        //-----------------------------------------------------------------------------
        SQRESULT GetPromoData(HSQUIRRELVM v)
        {
            enum class R5RPromoData : SQInteger
            {
                PromoLargeTitle,
                PromoLargeDesc,
                PromoLeftTitle,
                PromoLeftDesc,
                PromoRightTitle,
                PromoRightDesc
            };

            R5RPromoData ePromoIndex = static_cast<R5RPromoData>(sq_getinteger(v, 1));
            const char* pszPromoKey;

            switch (ePromoIndex)
            {
            case R5RPromoData::PromoLargeTitle:
            {
                pszPromoKey = "#PROMO_LARGE_TITLE";
                break;
            }
            case R5RPromoData::PromoLargeDesc:
            {
                pszPromoKey = "#PROMO_LARGE_DESCRIPTION";
                break;
            }
            case R5RPromoData::PromoLeftTitle:
            {
                pszPromoKey = "#PROMO_LEFT_TITLE";
                break;
            }
            case R5RPromoData::PromoLeftDesc:
            {
                pszPromoKey = "#PROMO_LEFT_DESCRIPTION";
                break;
            }
            case R5RPromoData::PromoRightTitle:
            {
                pszPromoKey = "#PROMO_RIGHT_TITLE";
                break;
            }
            case R5RPromoData::PromoRightDesc:
            {
                pszPromoKey = "#PROMO_RIGHT_DESCRIPTION";
                break;
            }
            default:
            {
                pszPromoKey = "#PROMO_SDK_ERROR";
                break;
            }
            }

            sq_pushstring(v, pszPromoKey, -1);
            return SQ_OK;
        }

        SQRESULT GetEULAContents(HSQUIRRELVM v)
        {
            MSEulaData_t eulaData;
            string eulaRequestMessage;

            if (g_pMasterServer->GetEULA(eulaData, eulaRequestMessage))
            {
                // set EULA version cvar to the newly fetched EULA version
                eula_version->SetValue(eulaData.version);

                sq_pushstring(v, eulaData.contents.c_str(), -1);
            }
            else
            {
                string error = Format("Failed to load EULA Data: %s", eulaRequestMessage.c_str());

                Warning(eDLL_T::UI, "%s\n", error.c_str());
                sq_pushstring(v, error.c_str(), -1);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: connect to server from native server browser entries
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToServer(HSQUIRRELVM v)
        {
            SQChar* ipAddress = sq_getstring(v, 1);
            SQChar* cryptoKey = sq_getstring(v, 2);

            if (!VALID_CHARSTAR(ipAddress) || VALID_CHARSTAR(cryptoKey))
                return SQ_OK;

            Msg(eDLL_T::UI, "Connecting to server with ip address '%s' and encryption key '%s'\n", ipAddress, cryptoKey);
            g_pServerListManager->ConnectToServer(ipAddress, cryptoKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: set netchannel encryption key and connect to server
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToListedServer(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
            SQInteger iServer = sq_getinteger(v, 1);

            if (!Script_CheckServerIndex(v, iServer))
            {
                return SQ_ERROR;
            }

            const NetGameServer_t& gameServer = g_pServerListManager->m_vServerList[iServer];

            g_pServerListManager->ConnectToServer(gameServer.m_svIpAddress, gameServer.m_nGamePort,
                gameServer.m_svEncryptionKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: request token from pylon and join server with result.
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToHiddenServer(HSQUIRRELVM v)
        {
            SQChar* privateToken = sq_getstring(v, 1);

            if (!VALID_CHARSTAR(privateToken))
                return SQ_OK;

            string hiddenServerRequestMessage;
            NetGameServer_t netListing;

            bool result = g_pMasterServer->GetServerByToken(netListing, hiddenServerRequestMessage, privateToken); // Send token connect request.
            if (result)
            {
                g_pServerListManager->ConnectToServer(netListing.m_svIpAddress, netListing.m_nGamePort, netListing.m_svEncryptionKey);
            }
            else
            {
                Warning(eDLL_T::UI, "Failed to connect to private server: %s\n", hiddenServerRequestMessage.c_str());
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: checks whether this SDK build is a client dll
        //-----------------------------------------------------------------------------
        SQRESULT IsClientDLL(HSQUIRRELVM v)
        {
            sq_pushbool(v, ::IsClientDLL());
            return SQ_OK;
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterClientFunctions(CSquirrelVM* s)
{
    Script_RegisterCommonAbstractions(s);
    Script_RegisterCoreClientFunctions(s);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterUIFunctions(CSquirrelVM* s)
{
    Script_RegisterCommonAbstractions(s);
    Script_RegisterCoreClientFunctions(s);

    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, RefreshServerList, "Refreshes the public server list and returns the count", "int", "");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerCount, "Gets the number of public servers", "int", "");

    // Functions for retrieving server browser data
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetHiddenServerName, "Gets hidden server name by token", "string", "string");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerName, "Gets the name of the server at the specified index of the server list", "string", "int");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerDescription, "Gets the description of the server at the specified index of the server list", "string", "int");

    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerMap, "Gets the map of the server at the specified index of the server list", "string", "int");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerPlaylist, "Gets the playlist of the server at the specified index of the server list", "string", "int");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerCurrentPlayers, "Gets the current player count of the server at the specified index of the server list", "int", "int");

    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetServerMaxPlayers, "Gets the max player count of the server at the specified index of the server list", "int", "int");

    // Misc main menu functions
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetPromoData, "Gets promo data for specified slot type", "string", "int");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, GetEULAContents, "Gets EULA contents from masterserver", "string", "");

    // Functions for connecting to servers
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, ConnectToServer, "Joins server by ip address and encryption key", "void", "string, string");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, ConnectToListedServer, "Joins listed server by index", "void", "int");
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, ConnectToHiddenServer, "Joins hidden server by token", "void", "string");
}

//---------------------------------------------------------------------------------
// Purpose: core client script functions
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCoreClientFunctions(CSquirrelVM* s)
{
    DEFINE_CLIENT_SCRIPTFUNC_NAMED(s, IsClientDLL, "Returns whether this build is client only", "bool", "");
}
