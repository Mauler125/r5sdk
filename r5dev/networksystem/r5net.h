#pragma once

#include "net_structs.h"
#include "core/stdafx.h"

namespace R5Net
{
#define R5NET_GET_ENDPOINT(FuncName, Route, ResponseStructType)\
ResponseStructType FuncName() {\
	ResponseStructType result{};\
	httplib::Result response = HttpClient.Get(Route);\
	if(response == nullptr) return result;\
	switch(response->status)\
	{\
		case 403:\
		{\
			result.status = EResponseStatus::FORBIDDEN;\
			break;\
		}\
		case 404:\
		{\
			result.status = EResponseStatus::NOT_FOUND;\
			break;\
		}\
		case 500:\
		{\
			result.status = EResponseStatus::MS_ERROR;\
			break;\
		}\
		case 200:\
		{\
			result.status = EResponseStatus::SUCCESS;\
			break;\
		}\
		default: \
		{\
			result.status = EResponseStatus::UNKNOWN;\
			break;\
		}\
	}\
	nlohmann::json response_body = nlohmann::json::parse(response->body);\
	from_json(response_body, result);\
	return result;\
}\
\
void FuncName##_Async(std::function<void(ResponseStructType)> Callback) {\
	std::thread t([&]() {\
		Callback(FuncName());\
	});\
	t.detach();\
}\

#define R5NET_POST_ENDPOINT(FuncName, Route, RequestStructType, ResponseStructType)\
	ResponseStructType FuncName(const RequestStructType& request) {\
	ResponseStructType result{};\
	nlohmann::json request_body;\
	to_json(request_body, request);\
	httplib::Result response = HttpClient.Post(Route, request_body.dump(), "application/json");\
	if (response == nullptr) return result;\
	switch(response->status)\
	{\
		case 403:\
		{\
			result.status = EResponseStatus::FORBIDDEN;\
			break;\
		}\
		case 404:\
		{\
			result.status = EResponseStatus::NOT_FOUND;\
			break;\
		}\
		case 500:\
		{\
			result.status = EResponseStatus::MS_ERROR;\
			break;\
		}\
		case 200:\
		{\
			result.status = EResponseStatus::SUCCESS;\
			break;\
		}\
		default: \
		{\
			result.status = EResponseStatus::UNKNOWN;\
			break;\
		}\
	}\
	nlohmann::json response_body = nlohmann::json::parse(response->body);\
	from_json(response_body, result);\
	return result;\
}\
\
void FuncName##_Async(const RequestStructType& request, std::function<void(ResponseStructType)> Callback) {\
	std::thread t([&]() {\
		Callback(FuncName(request));\
	});\
	t.detach();\
}\


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
		R5NET_POST_ENDPOINT(GetPrivateGameServerInfo, "/api/game_servers/game_server_private_info", GetPrivateGameServerInfoMSRequest, GetPrivateGameServerInfoMSResponse)
		R5NET_POST_ENDPOINT(GetClientIsBanned, "/api/ban_system/is_user_banned", GetIsUserBannedMSRequest, GetIsUserBannedMSResponse)



	};

	extern NetGameServer* LocalServer;
}

extern R5Net::Client* g_pR5net;	