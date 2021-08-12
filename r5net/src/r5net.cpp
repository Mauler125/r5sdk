// r5net.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "r5net.h"


using namespace R5Net;

std::vector<ServerListing> Client::GetServersList()
{
    std::vector<ServerListing> list;
    
    auto res = m_HttpClient.Get("/servers");

    if (res)
    {
        nlohmann::json root = nlohmann::json::parse(res->body);
        for (auto obj : root["servers"])
        {
            list.push_back(
               ServerListing{ obj["name"].get<std::string>(), obj["map"].get<std::string>(), obj["ip"].get<std::string>(), obj["port"].get<std::string>() }
            );
        }
    }

    return list;
}

bool Client::PostServerHost(std::string& outMessage, std::string& outToken, const ServerListing& serverListing)
{
    nlohmann::json reqBody = nlohmann::json::object();
    reqBody["name"] = serverListing.name;
    reqBody["map"] = serverListing.map;
    reqBody["port"] = serverListing.port;
    reqBody["password"] = serverListing.password;

    std::string reqBodyStr = reqBody.dump();

    auto res = m_HttpClient.Post("/servers/add", reqBodyStr.c_str(), reqBodyStr.length(), "application/json");

    
    nlohmann::json resBody = nlohmann::json::parse(res->body);
    if (res && resBody["success"].is_boolean() && resBody["success"])
    {
        outMessage = "Broadcasting!";

        if (resBody["token"].is_string())
            outToken = resBody["token"].get<std::string>();
        else
            outToken     = "";
        return true;
    }
    else
    {
        if (resBody["err"].is_string())
            outMessage = resBody["err"].get<std::string>();
        else 
            outMessage = "An unknown error occured!";

        return false;
    }
}

bool R5Net::Client::GetServerByToken(ServerListing& outServer, std::string& outError, const std::string& token, const std::string& password)
{
    nlohmann::json reqBody = nlohmann::json::object();

    reqBody["token"] = token;
    reqBody["password"] = password;

    httplib::Result res = m_HttpClient.Post("/server/byToken", reqBody.dump().c_str(), reqBody.dump().length(), "application/json");

    std::cout << "YEEEEEEEEEEEEEE" << res->body << "\n";
    nlohmann::json resBody = nlohmann::json::parse(res->body);

    if (res && resBody["success"].is_boolean() && resBody["success"])
    {
        outServer = ServerListing{
            reqBody["name"].get<std::string>(),
            reqBody["map"].get<std::string>(),
            reqBody["ip"].get<std::string>(),
            reqBody["port"].get<std::string>()
        };
        return true;
    }
    else 
    {
        if (resBody["err"].is_string())
            outError = resBody["err"].get<std::string>();
        else
            outError = "";

        outServer = ServerListing{};
        return false;
    }
}

