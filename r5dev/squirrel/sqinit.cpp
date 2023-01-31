//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// Create functions here under the target VM namespace. If the function has to
// be registered for 2 or more VM's, put them under the 'SHARED' namespace. 
// Ifdef them out for 'DEDICATED' / 'CLIENT_DLL' if the target VM's do not 
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
#include "squirrel/sqtype.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqinit.h"
#include "networksystem/pylon.h"
#ifndef CLIENT_DLL
#include "networksystem/bansystem.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "networksystem/listmanager.h"
#endif // !DEDICATED

namespace VSquirrel
{
    namespace SHARED
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
            std::lock_guard<std::mutex> l(g_MapVecMutex);

            if (g_vAllMaps.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (const string& it : g_vAllMaps)
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
#ifndef CLIENT_DLL
        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerByName(HSQUIRRELVM v)
        {
            SQChar* szName = sq_getstring(v, 1);
            g_pBanSystem->KickPlayerByName(szName);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given handle or id
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerById(HSQUIRRELVM v)
        {
            SQChar* szHandle = sq_getstring(v, 1);
            g_pBanSystem->KickPlayerById(szHandle);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: bans a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT BanPlayerByName(HSQUIRRELVM v)
        {
            SQChar* szName = sq_getstring(v, 1);
            g_pBanSystem->BanPlayerByName(szName);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: bans a player by given handle or id
        //-----------------------------------------------------------------------------
        SQRESULT BanPlayerById(HSQUIRRELVM v)
        {
            SQChar* szHandle = sq_getstring(v, 1);
            g_pBanSystem->BanPlayerById(szHandle);

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
#ifndef DEDICATED
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
#endif // !DEDICATED
    }
#ifndef CLIENT_DLL
    namespace SERVER
    {
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
        // Purpose: checks whether this SDK build is a dedicated server
        //-----------------------------------------------------------------------------
        SQRESULT IsDedicated(HSQUIRRELVM v)
        {
            sq_pushbool(v, *s_bIsDedicated);
            return SQ_OK;
        }
    }
#endif // !CLIENT_DLL
#ifndef DEDICATED
    namespace CLIENT
    {
    }
    namespace UI
    {
        //-----------------------------------------------------------------------------
        // Purpose: refreshes the server list
        //-----------------------------------------------------------------------------
        SQRESULT RefreshServerCount(HSQUIRRELVM v)
        {
            string svMessage; // Refresh svListing list.
            size_t iCount = g_pServerListManager->RefreshServerList(svMessage);

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

            string svServerName = g_pServerListManager->m_vServerList[iServer].m_svHostName;
            sq_pushstring(v, svServerName.c_str(), -1);

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

            string svServerDescription = g_pServerListManager->m_vServerList[iServer].m_svDescription;
            sq_pushstring(v, svServerDescription.c_str(), -1);

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

            string svServerMapName = g_pServerListManager->m_vServerList[iServer].m_svHostMap;
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

            string svServerPlaylist = g_pServerListManager->m_vServerList[iServer].m_svPlaylist;
            sq_pushstring(v, svServerPlaylist.c_str(), -1);

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

            sq_pushinteger(v, strtol(g_pServerListManager->m_vServerList[iServer].m_svPlayerCount.c_str(), NULL, NULL));

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

            sq_pushinteger(v, strtol(g_pServerListManager->m_vServerList[iServer].m_svMaxPlayers.c_str(), NULL, NULL));

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
            string svPromo;

            switch (ePromoIndex)
            {
            case R5RPromoData::PromoLargeTitle:
            {
                svPromo = "#PROMO_LARGE_TITLE";
                break;
            }
            case R5RPromoData::PromoLargeDesc:
            {
                svPromo = "#PROMO_LARGE_DESCRIPTION";
                break;
            }
            case R5RPromoData::PromoLeftTitle:
            {
                svPromo = "#PROMO_LEFT_TITLE";
                break;
            }
            case R5RPromoData::PromoLeftDesc:
            {
                svPromo = "#PROMO_LEFT_DESCRIPTION";
                break;
            }
            case R5RPromoData::PromoRightTitle:
            {
                svPromo = "#PROMO_RIGHT_TITLE";
                break;
            }
            case R5RPromoData::PromoRightDesc:
            {
                svPromo = "#PROMO_RIGHT_DESCRIPTION";
                break;
            }
            default:
            {
                svPromo = "#PROMO_SDK_ERROR";
                break;
            }
            }

            sq_pushstring(v, svPromo.c_str(), -1);
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        //-----------------------------------------------------------------------------
        SQRESULT CreateServer(HSQUIRRELVM v)
        {
#ifndef CLIENT_DLL
            string svServerName = sq_getstring(v, 1);
            string svServerDescription = sq_getstring(v, 2);
            string svServerMapName = sq_getstring(v, 3);
            string svServerPlaylist = sq_getstring(v, 4);
            EServerVisibility_t eServerVisibility = static_cast<EServerVisibility_t>(sq_getinteger(v, 5));

            if (svServerName.empty() || svServerMapName.empty() || svServerPlaylist.empty())
                return SQ_OK;

            // Adjust browser settings.
            std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);

            g_pServerListManager->m_Server.m_svHostName = svServerName;
            g_pServerListManager->m_Server.m_svDescription = svServerDescription;
            g_pServerListManager->m_Server.m_svHostMap = svServerMapName;
            g_pServerListManager->m_Server.m_svPlaylist = svServerPlaylist;
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
            string svIpAddr = sq_getstring(v, 1);
            string svEncKey = sq_getstring(v, 2);

            if (svIpAddr.empty() || svEncKey.empty())
                return SQ_OK;

            DevMsg(eDLL_T::UI, "Connecting to server with ip address '%s' and encryption key '%s'\n", svIpAddr.c_str(), svEncKey.c_str());
            g_pServerListManager->ConnectToServer(svIpAddr, svEncKey);

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
            string svHiddenServerRequestMessage;
            string svToken = sq_getstring(v, 1);

            NetGameServer_t svListing;
            bool result = g_pMasterServer->GetServerByToken(svListing, svHiddenServerRequestMessage, svToken); // Send szToken connect request.
            if (result)
            {
                g_pServerListManager->ConnectToServer(svListing.m_svIpAddress, svListing.m_svGamePort, svListing.m_svEncryptionKey);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get response from private server request
        //-----------------------------------------------------------------------------
        SQRESULT GetHiddenServerConnectStatus(HSQUIRRELVM v)
        {
            string svHiddenServerRequestMessage;
            string svToken = sq_getstring(v, 1);

            NetGameServer_t serverListing;
            bool result = g_pMasterServer->GetServerByToken(serverListing, svHiddenServerRequestMessage, svToken); // Send token connect request.
            if (!serverListing.m_svHostName.empty())
            {
                svHiddenServerRequestMessage = fmt::format("Found server: {:s}", serverListing.m_svHostName);
                sq_pushstring(v, svHiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                svHiddenServerRequestMessage = "Server not found";
                sq_pushstring(v, svHiddenServerRequestMessage.c_str(), -1);
            }

            return SQ_OK;
        }
    }
#endif // !DEDICATED
}