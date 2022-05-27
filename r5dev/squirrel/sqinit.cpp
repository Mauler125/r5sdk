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
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // CLIENT_DLL
#include "engine/cmodel_bsp.h"
#include "squirrel/sqtype.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqinit.h"
#include "networksystem/r5net.h"

#ifndef DEDICATED
#include "gameui/IBrowser.h" // TODO: create dedicated class for exposing server utils to ImGui and UI VM.
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
            sq_pushstring(v, g_pR5net->GetSDKVersion().c_str(), -1);
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(HSQUIRRELVM v)
        {
            if (g_vAllMaps.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (auto& it : g_vAllMaps)
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
            if (g_vAllPlaylists.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (auto& it : g_vAllPlaylists)
            {
                sq_pushstring(v, it.c_str(), -1);
                sq_arrayappend(v, -2);
            }

            return SQ_OK;
        }
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
    }
#endif // !CLIENT_DLL
#ifndef DEDICATED
    namespace CLIENT
    {
    }
    namespace UI
    {
        //-----------------------------------------------------------------------------
        // Purpose: get server's current name from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerName(HSQUIRRELVM v)
        {
            int iServerIndex = sq_getinteger(v, 1);
            std::string svServerName = g_pBrowser->m_vServerList[iServerIndex].svServerName;

            sq_pushstring(v, svServerName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current playlist via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerPlaylist(HSQUIRRELVM v)
        {
            int iServerIndex = sq_getinteger(v, 1);
            std::string svServerPlaylist = g_pBrowser->m_vServerList[iServerIndex].svPlaylist;

            sq_pushstring(v, svServerPlaylist.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current map via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMap(HSQUIRRELVM v)
        {
            int iServerIndex = sq_getinteger(v, 1);
            std::string svServerMapName = g_pBrowser->m_vServerList[iServerIndex].svMapName;

            sq_pushstring(v, svServerMapName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get current server count from pylon
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCount(HSQUIRRELVM v)
        {
            g_pBrowser->GetServerList(); // Refresh svListing list.

            sq_pushinteger(v, g_pBrowser->m_vServerList.size());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get promo data for serverbrowser panels
        //-----------------------------------------------------------------------------
        SQRESULT GetPromoData(HSQUIRRELVM v)
        {
            enum class R5RPromoData : int
            {
                PromoLargeTitle,
                PromoLargeDesc,
                PromoLeftTitle,
                PromoLeftDesc,
                PromoRightTitle,
                PromoRightDesc
            };

            R5RPromoData ePromoIndex = (R5RPromoData)sq_getinteger(v, 1);

            std::string svPromo = std::string();

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
        // Purpose: set netchannel encryption key and connect to server
        //-----------------------------------------------------------------------------
        SQRESULT SetEncKeyAndConnect(HSQUIRRELVM v)
        {
            int iServerIndex = sq_getinteger(v, 1);

            // !TODO: Create glue class instead.
            g_pBrowser->ConnectToServer(g_pBrowser->m_vServerList[iServerIndex].svIpAddress, g_pBrowser->m_vServerList[iServerIndex].svPort, g_pBrowser->m_vServerList[iServerIndex].svEncryptionKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        //-----------------------------------------------------------------------------
        SQRESULT CreateServerFromMenu(HSQUIRRELVM v)
        {
            std::string svServerName = sq_getstring(v, 1);
            std::string svServerMapName = sq_getstring(v, 2);
            std::string svServerPlaylist = sq_getstring(v, 3);
            EServerVisibility eServerVisibility = (EServerVisibility)sq_getinteger(v, 4);

            if (svServerName.empty() || svServerMapName.empty() || svServerPlaylist.empty())
                return SQ_OK;

            // Adjust browser settings.
            g_pBrowser->m_Server.svPlaylist = svServerPlaylist;
            g_pBrowser->m_Server.svMapName = svServerMapName;
            g_pBrowser->m_Server.svServerName = svServerName;
            g_pBrowser->eServerVisibility = eServerVisibility;

            // Launch server.
            g_pBrowser->LaunchServer();

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: request token from pylon and join server with result.
        //-----------------------------------------------------------------------------
        SQRESULT JoinPrivateServerFromMenu(HSQUIRRELVM v)
        {
            std::string svHiddenServerRequestMessage = std::string();

            std::string svToken = sq_getstring(v, 1);

            ServerListing svListing;
            bool result = g_pR5net->GetServerByToken(svListing, svHiddenServerRequestMessage, svToken); // Send szToken connect request.
            if (result)
            {
                g_pBrowser->ConnectToServer(svListing.svIpAddress, svListing.svPort, svListing.svEncryptionKey);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get response from private server request
        //-----------------------------------------------------------------------------
        SQRESULT GetPrivateServerMessage(HSQUIRRELVM v)
        {
            std::string svHiddenServerRequestMessage = std::string();

            std::string svToken = sq_getstring(v, 1);

            ServerListing serverListing;
            bool result = g_pR5net->GetServerByToken(serverListing, svHiddenServerRequestMessage, svToken); // Send szToken connect request.
            if (!serverListing.svServerName.empty())
            {
                svHiddenServerRequestMessage = "Found Server: " + serverListing.svServerName;

                sq_pushstring(v, svHiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                svHiddenServerRequestMessage = "Error: Server Not Found";

                sq_pushstring(v, svHiddenServerRequestMessage.c_str(), -1);
            }

            DevMsg(eDLL_T::UI, "GetPrivateServeMessage response: %s\n", svHiddenServerRequestMessage.c_str());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: connect to server from native server browser entries
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToIPFromMenu(HSQUIRRELVM v)
        {
            std::string svIpAddr = sq_getstring(v, 1);
            std::string svEncKey = sq_getstring(v, 2);

            if (svIpAddr.empty() || svEncKey.empty())
                return SQ_OK;

            DevMsg(eDLL_T::UI, "Connecting to server with ip-address '%s' and encryption key '%s'\n", svIpAddr.c_str(), svEncKey.c_str());

            g_pBrowser->ConnectToServer(svIpAddr, svEncKey);

            return SQ_OK;
        }
    }
#endif // !DEDICATED
}