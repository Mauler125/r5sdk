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
#include "engine/sys_utils.h"
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
        SQRESULT SDKNativeTest(HSQUIRRELVM* v)
        {
            // Function code goes here.
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: expose SDK version to the VScript API
        //-----------------------------------------------------------------------------
        SQRESULT GetSDKVersion(HSQUIRRELVM* v)
        {
            sq_pushstring(v, g_pR5net->GetSDKVersion().c_str(), -1);
            return SQ_OK;
        }
    }
#ifndef CLIENT_DLL
    namespace SERVER
    {
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
        SQRESULT GetServerName(HSQUIRRELVM* v)
        {
            int iServerIndex = sq_getinteger(v, 1);
            std::string svServerName = g_pIBrowser->m_vServerList[iServerIndex].svServerName;

            sq_pushstring(v, svServerName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current playlist via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerPlaylist(HSQUIRRELVM* v)
        {
            int iServerIndex = sq_getinteger(v, 1);
            std::string svServerPlaylist = g_pIBrowser->m_vServerList[iServerIndex].svPlaylist;

            sq_pushstring(v, svServerPlaylist.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current map via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMap(HSQUIRRELVM* v)
        {
            int iServerIndex = sq_getinteger(v, 1);
            std::string svServerMapName = g_pIBrowser->m_vServerList[iServerIndex].svMapName;

            sq_pushstring(v, svServerMapName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get current server count from pylon
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCount(HSQUIRRELVM* v)
        {
            g_pIBrowser->GetServerList(); // Refresh svListing list.

            sq_pushinteger(v, g_pIBrowser->m_vServerList.size());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get promo data for serverbrowser panels
        //-----------------------------------------------------------------------------
        SQRESULT GetPromoData(HSQUIRRELVM* v)
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
                svPromo = "Welcome To R5Reloaded!";
                break;
            }
            case R5RPromoData::PromoLargeDesc:
            {
                svPromo = "Make sure to join the discord! discord.gg/r5reloaded";
                break;
            }
            case R5RPromoData::PromoLeftTitle:
            {
                svPromo = "Yes";
                break;
            }
            case R5RPromoData::PromoLeftDesc:
            {
                svPromo = "Your ad could be here";
                break;
            }
            case R5RPromoData::PromoRightTitle:
            {
                svPromo = "Yes2";
                break;
            }
            case R5RPromoData::PromoRightDesc:
            {
                svPromo = "Yes3";
                break;
            }
            default:
            {
                svPromo = "You should not see this.";
                break;
            }
            }

            sq_pushstring(v, svPromo.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: set netchannel encryption key and connect to server
        //-----------------------------------------------------------------------------
        SQRESULT SetEncKeyAndConnect(HSQUIRRELVM* v)
        {
            int iServerIndex = sq_getinteger(v, 1);

            // !TODO: Create glue class instead.
            g_pIBrowser->ConnectToServer(g_pIBrowser->m_vServerList[iServerIndex].svIpAddress, g_pIBrowser->m_vServerList[iServerIndex].svPort, g_pIBrowser->m_vServerList[iServerIndex].svEncryptionKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        //-----------------------------------------------------------------------------
        SQRESULT CreateServerFromMenu(HSQUIRRELVM* v)
        {
            std::string svServerName = sq_getstring(v, 1);
            std::string svServerMapName = sq_getstring(v, 2);
            std::string svServerPlaylist = sq_getstring(v, 3);
            EServerVisibility eServerVisibility = (EServerVisibility)sq_getinteger(v, 4);

            if (svServerName.empty() || svServerMapName.empty() || svServerPlaylist.empty())
                return SQ_OK;

            // Adjust browser settings.
            g_pIBrowser->m_Server.svPlaylist = svServerPlaylist;
            g_pIBrowser->m_Server.svMapName = svServerMapName;
            g_pIBrowser->m_Server.svServerName = svServerName;
            g_pIBrowser->eServerVisibility = eServerVisibility;

            // Launch server.
            g_pIBrowser->LaunchServer();

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: request token from pylon and join server with result.
        //-----------------------------------------------------------------------------
        SQRESULT JoinPrivateServerFromMenu(HSQUIRRELVM* v)
        {
            std::string svHiddenServerRequestMessage = std::string();

            std::string svToken = sq_getstring(v, 1);

            ServerListing svListing;
            bool result = g_pR5net->GetServerByToken(svListing, svHiddenServerRequestMessage, svToken); // Send szToken connect request.
            if (result)
            {
                g_pIBrowser->ConnectToServer(svListing.svIpAddress, svListing.svPort, svListing.svEncryptionKey);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get response from private server request
        //-----------------------------------------------------------------------------
        SQRESULT GetPrivateServerMessage(HSQUIRRELVM* v)
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
        SQRESULT ConnectToIPFromMenu(HSQUIRRELVM* v)
        {
            std::string svIpAddr = sq_getstring(v, 1);
            std::string svEncKey = sq_getstring(v, 2);

            if (svIpAddr.empty() || svEncKey.empty())
                return SQ_OK;

            DevMsg(eDLL_T::UI, "Connecting to server with ip-address '%s' and encryption key '%s'\n", svIpAddr.c_str(), svEncKey.c_str());

            g_pIBrowser->ConnectToServer(svIpAddr, svEncKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(HSQUIRRELVM* v)
        {
            std::vector<std::string> vsvMapList = g_pIBrowser->m_vszMapFileNameList;

            if (vsvMapList.empty())
            {
                Warning(eDLL_T::UI, "%s: Available maps is empty!\n", __FUNCTION__);
                return SQ_OK;
            }

            DevMsg(eDLL_T::UI, "Requesting an array of '%i' available maps from script\n", vsvMapList.size());

            sq_newarray(v, 0);

            for (auto& it : vsvMapList)
            {
                sq_pushstring(v, it.c_str(), -1);
                sq_arrayappend(v, -2);
            }

            return SQ_OK;
        }
    }
#endif // !DEDICATED
}