#pragma once
#include "serverlisting.h"

namespace R5Net
{
	class Client
	{
	public:
		Client(std::string masterServerConnectionString) : HttpClient(masterServerConnectionString.c_str()) 
		{
			HttpClient.set_connection_timeout(25);
		}


		R5NET_GET_ENDPOINT(GetGlobalStats, "/api/stats", GetGlobalStatsMSResponse)
		R5NET_GET_ENDPOINT(GetGameServersList, "/api/game_servers/list", GetGameServersListMSResponse)

		R5NET_POST_ENDPOINT(UpdateMyGameServer, "/api/game_servers/update", UpdateGameServerMSRequest, UpdateGameServerMSResponse)
		R5NET_POST_ENDPOINT(GetClientIsBanned, "/api/ban_system/is_user_banned", GetIsUserBannedMSRequest, GetIsUserBannedMSResponse)



	};
}
extern R5Net::Client* g_pR5net;
