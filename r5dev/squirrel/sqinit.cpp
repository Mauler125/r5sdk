//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// Create functions here under the target VM namespace. If the function has to
// be registered for 2 or more VM's, put them under the 'SHARED' namespace. 
// Ifdef them out for 'DEDICATED' if the target VM's do not include 'SERVER'.
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/sys_utils.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqinit.h"

#ifndef DEDICATED
#include "gameui/IBrowser.h" // TODO: create dedicated class for exposing server utils to ImGui and UI VM.
#endif // !DEDICATED

namespace VSquirrel
{
    namespace SHARED
    {
        SQRESULT Script_NativeTest(void* sqvm)
        {
            // Function code goes here.
            return SQ_OK;
        }
    }
    namespace SERVER
    {
    }
#ifndef DEDICATED
    namespace CLIENT
    {
    }
    namespace UI
    {
        //----------------------------------------------------------------------------
        // Purpose: get server's current name from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerName(void* sqvm)
        {
            int iServerIndex = hsq_getinteger(sqvm, 1);
            std::string svServerName = g_pIBrowser->m_vServerList[iServerIndex].svServerName;

            hsq_pushstring(sqvm, svServerName.c_str(), -1);

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: get server's current playlist via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerPlaylist(void* sqvm)
        {
            int iServerIndex = hsq_getinteger(sqvm, 1);
            std::string svServerPlaylist = g_pIBrowser->m_vServerList[iServerIndex].svPlaylist;

            hsq_pushstring(sqvm, svServerPlaylist.c_str(), -1);

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: get server's current map via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMap(void* sqvm)
        {
            int iServerIndex = hsq_getinteger(sqvm, 1);
            std::string svServerMapName = g_pIBrowser->m_vServerList[iServerIndex].svMapName;

            hsq_pushstring(sqvm, svServerMapName.c_str(), -1);

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: get current server count from pylon
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCount(void* sqvm)
        {
            g_pIBrowser->GetServerList(); // Refresh svListing list.

            hsq_pushinteger(sqvm, g_pIBrowser->m_vServerList.size());

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: expose SDK version to the UI VM
        //-----------------------------------------------------------------------------
        SQRESULT GetSDKVersion(void* sqvm)
        {
            hsq_pushstring(sqvm, g_pR5net->GetSDKVersion().c_str(), -1);

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: get promo data for serverbrowser panels
        //-----------------------------------------------------------------------------
        SQRESULT GetPromoData(void* sqvm)
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

            R5RPromoData ePromoIndex = (R5RPromoData)hsq_getinteger(sqvm, 1);

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

            hsq_pushstring(sqvm, svPromo.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: set netchannel encryption key and connect to server
        //-----------------------------------------------------------------------------
        SQRESULT SetEncKeyAndConnect(void* sqvm)
        {
            int iServerIndex = hsq_getinteger(sqvm, 1);

            g_pIBrowser->ConnectToServer(g_pIBrowser->m_vServerList[iServerIndex].svIpAddress, g_pIBrowser->m_vServerList[iServerIndex].svPort, g_pIBrowser->m_vServerList[iServerIndex].svEncryptionKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        //-----------------------------------------------------------------------------
        SQRESULT CreateServerFromMenu(void* sqvm)
        {
            std::string svServerName = hsq_getstring(sqvm, 1);
            std::string svServerMapName = hsq_getstring(sqvm, 2);
            std::string svServerPlaylist = hsq_getstring(sqvm, 3);
            EServerVisibility eServerVisibility = (EServerVisibility)hsq_getinteger(sqvm, 4);

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
        SQRESULT JoinPrivateServerFromMenu(void* sqvm)
        {
            std::string svHiddenServerRequestMessage = std::string();

            std::string svToken = hsq_getstring(sqvm, 1);

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
        SQRESULT GetPrivateServerMessage(void* sqvm)
        {
            std::string svHiddenServerRequestMessage = std::string();

            std::string svToken = hsq_getstring(sqvm, 1);

            ServerListing serverListing;
            bool result = g_pR5net->GetServerByToken(serverListing, svHiddenServerRequestMessage, svToken); // Send szToken connect request.
            if (!serverListing.svServerName.empty())
            {
                svHiddenServerRequestMessage = "Found Server: " + serverListing.svServerName;

                hsq_pushstring(sqvm, svHiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                svHiddenServerRequestMessage = "Error: Server Not Found";

                hsq_pushstring(sqvm, svHiddenServerRequestMessage.c_str(), -1);
            }

            DevMsg(eDLL_T::UI, "GetPrivateServeMessage response: %s\n", svHiddenServerRequestMessage.c_str());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: connect to server from native server browser entries
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToIPFromMenu(void* sqvm)
        {
            std::string svIpAddr = hsq_getstring(sqvm, 1);
            std::string svEncKey = hsq_getstring(sqvm, 2);

            if (svIpAddr.empty() || svEncKey.empty())
                return SQ_OK;

            DevMsg(eDLL_T::UI, "Connecting to server with connection string '%s' and encryption key '%s'\n", svIpAddr.c_str(), svEncKey.c_str());

            g_pIBrowser->ConnectToServer(svIpAddr, svEncKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(void* sqvm)
        {
            std::vector<std::string> vsvMapList = g_pIBrowser->m_vszMapFileNameList;

            if (vsvMapList.empty())
            {
                DevMsg(eDLL_T::UI, "Available maps is empty!!!\n");
                return SQ_OK;
            }

            DevMsg(eDLL_T::UI, "Requesting an array of '%i' available maps from script\n", vsvMapList.size());

            hsq_newarray(sqvm, 0);

            for (auto& it : vsvMapList)
            {
                hsq_pushstring(sqvm, it.c_str(), -1);
                hsq_arrayappend(sqvm, -2);
            }

            return SQ_OK;
        }
    }
#endif // !DEDICATED
}