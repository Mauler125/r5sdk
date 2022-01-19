#include "core/stdafx.h"
#include "squirrel/sqnativefunctions.h"

#include "engine/sys_utils.h"
#include "gameui/IBrowser.h"
#include <networksystem/r5net.h>

namespace SQNativeFunctions
{
	namespace IBrowser
	{

        //----------------------------------------------------------------------------
        // Purpose: get servers current server name from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerName(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string szSvName = g_pIBrowser->m_vServerList[svIndex].name;

            hsq_pushstring(sqvm, szSvName.c_str(), -1);

            return SQ_OK;
        }

        SQRESULT GetServerDescription(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string szSvName = g_pIBrowser->m_vServerList[svIndex].description;

            hsq_pushstring(sqvm, szSvName.c_str(), -1);

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: get servers current playlist via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerPlaylist(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string szSvPlaylist = g_pIBrowser->m_vServerList[svIndex].playlist;

            hsq_pushstring(sqvm, szSvPlaylist.c_str(), -1);

            return SQ_OK;
        }

        //----------------------------------------------------------------------------
        // Purpose: get servers current map via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMap(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string szSvMapName = g_pIBrowser->m_vServerList[svIndex].mapName;

            hsq_pushstring(sqvm, szSvMapName.c_str(), -1);

            return SQ_OK;
        }

        SQRESULT GetServerPlayersNum(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            int szSvPlayersNum = g_pIBrowser->m_vServerList[svIndex].playerCount;

            hsq_pushinteger(sqvm, szSvPlayersNum);

            return SQ_OK;
        }

        SQRESULT GetServerMaxPlayersNum(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            int szSvMaxPlayersNum = g_pIBrowser->m_vServerList[svIndex].maxPlayerCount;

            hsq_pushinteger(sqvm, szSvMaxPlayersNum);

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
        // Purpose: expose SDK version to SQ
        //-----------------------------------------------------------------------------
        SQRESULT GetSDKVersion(void* sqvm)
        {
            hsq_pushstring(sqvm, /*g_pR5net->GetSDKVersion().c_str()*/ "TEMP TEMP!!", -1);

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

            std::string szPromo = std::string();

            switch (ePromoIndex)
            {
            case R5RPromoData::PromoLargeTitle:
            {
                szPromo = "Welcome To R5Reloaded!";
                break;
            }
            case R5RPromoData::PromoLargeDesc:
            {
                szPromo = "Make sure to join the discord! discord.gg/r5reloaded";
                break;
            }
            case R5RPromoData::PromoLeftTitle:
            {
                szPromo = "Yes";
                break;
            }
            case R5RPromoData::PromoLeftDesc:
            {
                szPromo = "Your ad could be here";
                break;
            }
            case R5RPromoData::PromoRightTitle:
            {
                szPromo = "Yes2";
                break;
            }
            case R5RPromoData::PromoRightDesc:
            {
                szPromo = "Yes3";
                break;
            }
            default:
            {
                szPromo = "You should not see this.";
                break;
            }
            }

            hsq_pushstring(sqvm, szPromo.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: set netchannel encryption key and connect to server
        //-----------------------------------------------------------------------------
        SQRESULT SetEncKeyAndConnect(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);

            g_pIBrowser->ConnectToServer(g_pIBrowser->m_vServerList[svIndex].ipAddress, g_pIBrowser->m_vServerList[svIndex].gamePort, g_pIBrowser->m_vServerList[svIndex].encryptionKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        //-----------------------------------------------------------------------------
        SQRESULT CreateServerFromMenu(void* sqvm)
        {
            std::string szSvName        = hsq_getstring(sqvm, 1);
            std::string szSvDescription = hsq_getstring(sqvm, 2);
            std::string szSvMapName     = hsq_getstring(sqvm, 3);
            std::string szSvPlaylist    = hsq_getstring(sqvm, 4);
            std::string szSvPassword    = hsq_getstring(sqvm, 5);

            // Visibility got deprecated in favor of a password system.
            //EServerVisibility eSvVisibility = (EServerVisibility)hsq_getinteger(sqvm, 4);

            if (szSvName.empty() || szSvMapName.empty() || szSvPlaylist.empty())
                return SQ_OK;
            
            // Adjust browser settings.
            R5Net::LocalServer->playlist       = szSvPlaylist;
            R5Net::LocalServer->mapName        = szSvMapName;
            R5Net::LocalServer->name           = szSvName;
            R5Net::LocalServer->description    = szSvDescription;
            R5Net::LocalServer->password       = szSvPassword;

            // Launch server.
            g_pIBrowser->LaunchServer();

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // // Purpose: request token from pylon and join server with result.
        //-----------------------------------------------------------------------------
        SQRESULT JoinPrivateServerFromMenu(void* sqvm)
        {
            /*std::string szHiddenServerRequestMessage = std::string();

            std::string szToken = hsq_getstring(sqvm, 1);

            ServerListing svListing;
            bool result = g_pR5net->GetServerByToken(svListing, szHiddenServerRequestMessage, szToken); // Send szToken connect request.
            if (result)
            {
                g_pIBrowser->ConnectToServer(svListing.svIpAddress, svListing.svPort, svListing.svEncryptionKey);
            }

            return SQ_OK;*/
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get response from private server request
        //-----------------------------------------------------------------------------
        SQRESULT GetPrivateServerMessage(void* sqvm)
        {
            /*std::string szHiddenServerRequestMessage = std::string();

            std::string szToken = hsq_getstring(sqvm, 1);

            ServerListing slServer;
            bool result = g_pR5net->GetServerByToken(slServer, szHiddenServerRequestMessage, szToken); // Send szToken connect request.
            if (!slServer.svServerName.empty())
            {
                szHiddenServerRequestMessage = "Found Server: " + slServer.svServerName;

                hsq_pushstring(sqvm, szHiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                szHiddenServerRequestMessage = "Error: Server Not Found";

                hsq_pushstring(sqvm, szHiddenServerRequestMessage.c_str(), -1);
            }

            DevMsg(eDLL_T::UI, "GetPrivateServeMessage response: %s\n", szHiddenServerRequestMessage.c_str());

            return SQ_OK;*/
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: connect to server from native server browser entries
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToIPFromMenu(void* sqvm)
        {
            std::string szIP = hsq_getstring(sqvm, 1);
            std::string szEncKey = hsq_getstring(sqvm, 2);

            if (szIP.empty() || szEncKey.empty())
                return SQ_OK;

            DevMsg(eDLL_T::UI, "Connecting to server with connection string %s and encryptionkey %s\n", szIP, szEncKey);

            g_pIBrowser->ConnectToServer(szIP, szEncKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available map
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(void* sqvm)
        {
            std::vector<std::string> mapList = g_pIBrowser->m_vszMapFileNameList;

            if (mapList.empty())
            {
                DevMsg(eDLL_T::UI, "Available maps is empty!!!\n");
                return SQ_OK;
            }

            DevMsg(eDLL_T::UI, "Requesting an array of %i available maps from script\n", mapList.size());

            hsq_newarray(sqvm, 0);

            for (auto& it : mapList)
            {
                hsq_pushstring(sqvm, it.c_str(), -1);
                hsq_arrayappend(sqvm, -2);
            }

            return SQ_OK;
        }
	}
}