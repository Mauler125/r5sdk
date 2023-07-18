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
#include "networksystem/pylon.h"
#include "networksystem/listmanager.h"
#include "game/shared/vscript_shared.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

namespace VScriptCode
{
    namespace Client
    {
    }
    namespace Ui
    {
        //-----------------------------------------------------------------------------
        // Purpose: refreshes the server list
        //-----------------------------------------------------------------------------
        SQRESULT RefreshServerCount(HSQUIRRELVM v)
        {
            string serverMessage; // Refresh list.
            size_t iCount = g_pServerListManager->RefreshServerList(serverMessage);

            sq_pushinteger(v, static_cast<SQInteger>(iCount));

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current name from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerName(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);

            SQInteger iServer = sq_getinteger(v, 1);
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
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
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
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
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
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
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
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
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
                return SQ_ERROR;
            }

            const string& playerCount = g_pServerListManager->m_vServerList[iServer].m_svPlayerCount.c_str();
            sq_pushinteger(v, strtol(playerCount.c_str(), NULL, NULL));

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current player count via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMaxPlayers(HSQUIRRELVM v)
        {
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);

            SQInteger iServer = sq_getinteger(v, 1);
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
                return SQ_ERROR;
            }

            const string& maxPlayers = g_pServerListManager->m_vServerList[iServer].m_svMaxPlayers;
            sq_pushinteger(v, strtol(maxPlayers.c_str(), NULL, NULL));

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

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        // TODO: return a boolean on failure instead of raising an error, so we could
        // determine from scripts whether or not to spin a local server, or connect
        // to a dedicated server (for disconnecting and loading the lobby, for example)
        //-----------------------------------------------------------------------------
        SQRESULT CreateServer(HSQUIRRELVM v)
        {
#ifndef CLIENT_DLL
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
            g_pServerListManager->LaunchServer();

            return SQ_OK;
#else
            v_SQVM_RaiseError(v, "\"%s\" is not supported for client builds.\n", "CreateServer");
            return SQ_ERROR;
#endif
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

            DevMsg(eDLL_T::UI, "Connecting to server with ip address '%s' and encryption key '%s'\n", ipAddress, cryptoKey);
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
            SQInteger iCount = static_cast<SQInteger>(g_pServerListManager->m_vServerList.size());

            if (iServer >= iCount)
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n", iCount);
                return SQ_ERROR;
            }

            const NetGameServer_t& gameServer = g_pServerListManager->m_vServerList[iServer];

            g_pServerListManager->ConnectToServer(gameServer.m_svIpAddress, gameServer.m_svGamePort,
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
                g_pServerListManager->ConnectToServer(netListing.m_svIpAddress, netListing.m_svGamePort, netListing.m_svEncryptionKey);
            }
            else
            {
                Warning(eDLL_T::UI, "Failed to connect to private server: %s\n", hiddenServerRequestMessage.c_str());
            }

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
    }
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterClientFunctions(CSquirrelVM* s)
{
    s->RegisterFunction("SDKNativeTest", "Script_SDKNativeTest", "Native CLIENT test function", "void", "", &VScriptCode::Shared::SDKNativeTest);
    s->RegisterFunction("GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VScriptCode::Shared::GetSDKVersion);

    s->RegisterFunction("GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VScriptCode::Shared::GetAvailableMaps);
    s->RegisterFunction("GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VScriptCode::Shared::GetAvailablePlaylists);

    s->RegisterFunction("ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VScriptCode::Shared::ShutdownHostGame);
    s->RegisterFunction("IsClientDLL", "Script_IsClientDLL", "Returns whether this build is client only", "bool", "", &VScriptCode::Shared::IsClientDLL);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterUIFunctions(CSquirrelVM* s)
{
    s->RegisterFunction("SDKNativeTest", "Script_SDKNativeTest", "Native UI test function", "void", "", &VScriptCode::Shared::SDKNativeTest);
    s->RegisterFunction("GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VScriptCode::Shared::GetSDKVersion);

    s->RegisterFunction("RefreshServerList", "Script_RefreshServerList", "Refreshes the public server list and returns the count", "int", "", &VScriptCode::Ui::RefreshServerCount);

    // Functions for retrieving server browser data
    s->RegisterFunction("GetServerName", "Script_GetServerName", "Gets the name of the server at the specified index of the server list", "string", "int", &VScriptCode::Ui::GetServerName);
    s->RegisterFunction("GetServerDescription", "Script_GetServerDescription", "Gets the description of the server at the specified index of the server list", "string", "int", &VScriptCode::Ui::GetServerDescription);
    s->RegisterFunction("GetServerMap", "Script_GetServerMap", "Gets the map of the server at the specified index of the server list", "string", "int", &VScriptCode::Ui::GetServerMap);
    s->RegisterFunction("GetServerPlaylist", "Script_GetServerPlaylist", "Gets the playlist of the server at the specified index of the server list", "string", "int", &VScriptCode::Ui::GetServerPlaylist);
    s->RegisterFunction("GetServerCurrentPlayers", "Script_GetServerCurrentPlayers", "Gets the current player count of the server at the specified index of the server list", "int", "int", &VScriptCode::Ui::GetServerCurrentPlayers);
    s->RegisterFunction("GetServerMaxPlayers", "Script_GetServerMaxPlayers", "Gets the max player count of the server at the specified index of the server list", "int", "int", &VScriptCode::Ui::GetServerMaxPlayers);
    s->RegisterFunction("GetServerCount", "Script_GetServerCount", "Gets the number of public servers", "int", "", &VScriptCode::Ui::GetServerCount);

    // Misc main menu functions
    s->RegisterFunction("GetPromoData", "Script_GetPromoData", "Gets promo data for specified slot type", "string", "int", &VScriptCode::Ui::GetPromoData);

    // Functions for creating servers
    s->RegisterFunction("CreateServer", "Script_CreateServer", "Starts server with the specified settings", "void", "string, string, string, string, int", &VScriptCode::Ui::CreateServer);
    s->RegisterFunction("IsServerActive", "Script_IsServerActive", "Returns whether the server is active", "bool", "", &VScriptCode::Shared::IsServerActive);

    // Functions for connecting to servers
    s->RegisterFunction("ConnectToServer", "Script_ConnectToServer", "Joins server by ip address and encryption key", "void", "string, string", &VScriptCode::Ui::ConnectToServer);
    s->RegisterFunction("ConnectToListedServer", "Script_ConnectToListedServer", "Joins listed server by index", "void", "int", &VScriptCode::Ui::ConnectToListedServer);
    s->RegisterFunction("ConnectToHiddenServer", "Script_ConnectToHiddenServer", "Joins hidden server by token", "void", "string", &VScriptCode::Ui::ConnectToHiddenServer);

    s->RegisterFunction("GetHiddenServerName", "Script_GetHiddenServerName", "Gets hidden server name by token", "string", "string", &VScriptCode::Ui::GetHiddenServerName);
    s->RegisterFunction("GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VScriptCode::Shared::GetAvailableMaps);
    s->RegisterFunction("GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VScriptCode::Shared::GetAvailablePlaylists);

#ifndef CLIENT_DLL // UI 'admin' functions controlling server code
    s->RegisterFunction("KickPlayerByName", "Script_KickPlayerByName", "Kicks a player from the server by name", "void", "string", &VScriptCode::SHARED::KickPlayerByName);
    s->RegisterFunction("KickPlayerById", "Script_KickPlayerById", "Kicks a player from the server by handle or nucleus id", "void", "string", &VScriptCode::SHARED::KickPlayerById);

    s->RegisterFunction("BanPlayerByName", "Script_BanPlayerByName", "Bans a player from the server by name", "void", "string", &VScriptCode::SHARED::BanPlayerByName);
    s->RegisterFunction("BanPlayerById", "Script_BanPlayerById", "Bans a player from the server by handle or nucleus id", "void", "string", &VScriptCode::SHARED::BanPlayerById);

    s->RegisterFunction("UnbanPlayer", "Script_UnbanPlayer", "Unbans a player from the server by nucleus id or ip address", "void", "string", &VScriptCode::SHARED::UnbanPlayer);

    s->RegisterFunction("ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VScriptCode::SHARED::ShutdownHostGame);
#endif // !CLIENT_DLL

    s->RegisterFunction("IsClientDLL", "Script_IsClientDLL", "Returns whether this build is client only", "bool", "", &VScriptCode::Shared::IsClientDLL);
}
