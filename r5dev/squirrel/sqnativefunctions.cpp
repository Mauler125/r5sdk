#include "core/stdafx.h"
#include "squirrel/sqnativefunctions.h"

#include "engine/sys_utils.h"
#include "gameui/IBrowser.h"

namespace SQNativeFunctions
{
	namespace IBrowser
	{
        SQRESULT GetServerName(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string svName = g_pIBrowser->m_vServerList[svIndex].svServerName;

            hsq_pushstring(sqvm, svName.c_str(), -1);

            return SQ_OK;
        }

        SQRESULT GetServerPlaylist(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string svPlaylist = g_pIBrowser->m_vServerList[svIndex].svPlaylist;

            hsq_pushstring(sqvm, svPlaylist.c_str(), -1);

            return SQ_OK;
        }

        SQRESULT GetServerMap(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);
            std::string svMapName = g_pIBrowser->m_vServerList[svIndex].svMapName;

            hsq_pushstring(sqvm, svMapName.c_str(), -1);

            return SQ_OK;
        }

        SQRESULT GetServerCount(void* sqvm)
        {
            g_pIBrowser->GetServerList(); // Refresh server list.

            hsq_pushinteger(sqvm, g_pIBrowser->m_vServerList.size());

            return SQ_OK;
        }

        SQRESULT GetSDKVersion(void* sqvm)
        {
            hsq_pushstring(sqvm, g_pR5net->GetSDKVersion().c_str(), -1);

            return SQ_OK;
        }

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

            R5RPromoData prIndex = (R5RPromoData)hsq_getinteger(sqvm, 1);

            std::string prStr = std::string();

            switch (prIndex)
            {
            case R5RPromoData::PromoLargeTitle:
            {
                prStr = "Welcome To R5Reloaded!";
                break;
            }
            case R5RPromoData::PromoLargeDesc:
            {
                prStr = "Make sure to join the discord! discord.gg/r5reloaded";
                break;
            }
            case R5RPromoData::PromoLeftTitle:
            {
                prStr = "Yes";
                break;
            }
            case R5RPromoData::PromoLeftDesc:
            {
                prStr = "Your ad could be here";
                break;
            }
            case R5RPromoData::PromoRightTitle:
            {
                prStr = "Yes2";
                break;
            }
            case R5RPromoData::PromoRightDesc:
            {
                prStr = "Yes3";
                break;
            }
            default:
            {
                prStr = "You should not see this.";
                break;
            }
            }

            hsq_pushstring(sqvm, prStr.c_str(), -1);

            return SQ_OK;
        }

        SQRESULT SetEncKeyAndConnect(void* sqvm)
        {
            int svIndex = hsq_getinteger(sqvm, 1);

            g_pIBrowser->ConnectToServer(g_pIBrowser->m_vServerList[svIndex].svIpAddress, g_pIBrowser->m_vServerList[svIndex].svPort, g_pIBrowser->m_vServerList[svIndex].svEncryptionKey);

            return SQ_OK;
        }

        SQRESULT CreateServerFromMenu(void* sqvm)
        {
            std::string svName = hsq_getstring(sqvm, 1);
            std::string svMapName = hsq_getstring(sqvm, 2);
            std::string svPlaylist = hsq_getstring(sqvm, 3);
            EServerVisibility svVisibility = (EServerVisibility)hsq_getinteger(sqvm, 4);

            if (svMapName.empty() || svPlaylist.empty())
                return SQ_OK;

            g_pIBrowser->SetMenuVars(svName, svVisibility); // Pass integer instead

            /* Changing this up to call a IBrowser method eventually. */
            DevMsg(eDLL_T::ENGINE, "Starting Server with map '%s' and playlist '%s'\n", svMapName.c_str(), svPlaylist.c_str());

            g_pIBrowser->LoadPlaylist(svPlaylist.c_str());
            std::stringstream cgmd;
            cgmd << "mp_gamemode " << svPlaylist;
            g_pIBrowser->ProcessCommand(cgmd.str().c_str());

            // This is to avoid svIndex race condition.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::stringstream cmd;
            cmd << "map " << svMapName;
            g_pIBrowser->ProcessCommand(cmd.str().c_str());

            return SQ_OK;
        }

        SQRESULT JoinPrivateServerFromMenu(void* sqvm)
        {
            std::string m_szHiddenServerRequestMessage;

            std::string token = hsq_getstring(sqvm, 1);

            ServerListing server;
            bool result = g_pR5net->GetServerByToken(server, m_szHiddenServerRequestMessage, token); // Send token connect request.
            if (result)
            {
                g_pIBrowser->ConnectToServer(server.svIpAddress, server.svPort, server.svEncryptionKey);
            }

            return SQ_OK;
        }

        SQRESULT GetPrivateServerMessage(void* sqvm)
        {
            std::string m_szHiddenServerRequestMessage;

            std::string token = hsq_getstring(sqvm, 1);

            ServerListing server;
            bool result = g_pR5net->GetServerByToken(server, m_szHiddenServerRequestMessage, token); // Send token connect request.
            if (!server.svServerName.empty())
            {
                m_szHiddenServerRequestMessage = "Found Server: " + server.svServerName;

                hsq_pushstring(sqvm, m_szHiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                m_szHiddenServerRequestMessage = "Error: Server Not Found";

                hsq_pushstring(sqvm, m_szHiddenServerRequestMessage.c_str(), -1);
            }

            return SQ_OK;
        }

        SQRESULT ConnectToIPFromMenu(void* sqvm)
        {
            std::string ip = hsq_getstring(sqvm, 1);
            std::string key = hsq_getstring(sqvm, 2);

            g_pIBrowser->ConnectToServer(ip, key);

            return SQ_OK;
        }

        SQRESULT GetAvailableMaps(void* sqvm)
        {
            std::vector<std::string> mapList = g_pIBrowser->m_vszMapFileNameList;

            DevMsg(eDLL_T::UI, "Requesting an array of %i available maps from script\n", mapList.size());

            hsq_newarray(sqvm, 0);

            for (int i = 0; i < mapList.size(); ++i)
            {
                hsq_pushstring(sqvm, mapList[i].c_str(), -1);
                hsq_arrayappend(sqvm, -2);
            }

            return SQ_OK;
        }
	}
}