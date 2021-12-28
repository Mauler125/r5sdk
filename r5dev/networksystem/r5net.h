#pragma once

#include "net_structs.h"
#include "core/stdafx.h"

namespace R5Net
{
#define R5NET_GET_ENDPOINT(FuncName, Route, ResponseStructType) ResponseStructType FuncName() {\
	ResponseStructType result{};\
	httplib::Result response = HttpClient.Get(Route);\
	if(response == nullptr) return result;\
	nlohmann::json response_body = nlohmann::json::parse(response->body);\
	nlohmann::to_json(response_body, result);\
	return result;\
}\

#define R5NET_POST_ENDPOINT(FuncName, Route, RequestStructType, ResponseStructType) ResponseStructType FuncName(RequestStructType& request) {\
	ResponseStructType result{};\
	nlohmann::json request_body;\
	nlohmann::to_json(request_body, request);\
	httplib::Result response = HttpClient.Post("/", request_body.dump(), "application/json");\
	if (response == nullptr) return result;\
	nlohmann::json response_body = nlohmann::json::parse(response->body);\
	nlohmann::to_json(response_body, result);\
	return result;\
}


	class Client 
	{
		httplib::Client HttpClient;

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