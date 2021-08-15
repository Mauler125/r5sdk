// r5net.cpp : Defines the functions for the static library.
//

#include "netpch.h"
#include "r5\r5net.h"

std::string R5Net::Client::GetVersionString()
{
    return "beta 1.5";
}

std::vector<ServerListing> R5Net::Client::GetServersList(std::string& outMessage)
{
    std::vector<ServerListing> list{ };

    nlohmann::json reqBody = nlohmann::json::object();
    reqBody["version"] = GetVersionString();

    std::string reqBodyStr = reqBody.dump();;
    
    httplib::Result res = m_HttpClient.Post("/servers", reqBody.dump().c_str(), reqBody.dump().length(), "application/json");

    if (res)
    {
        nlohmann::json resBody = nlohmann::json::parse(res->body);
        if (resBody["success"].is_boolean() && resBody["success"].get<bool>())
        {
            for (auto obj : resBody["servers"])
            {
                list.push_back(
                    ServerListing{ obj["name"].get<std::string>(), obj["map"].get<std::string>(), obj["ip"].get<std::string>(), obj["port"].get<std::string>(), obj["gamemode"].get<std::string>() }
                );
            }
        }
        else
        {
            if (resBody["err"].is_string())
                outMessage = resBody["err"].get<std::string>();
            else
                outMessage = "An unknown error occured!";
        }
    }
    else
    {
        outMessage = "Failed to reach comp-server";
    }

    return list;
}

bool R5Net::Client::PostServerHost(std::string& outMessage, std::string& outToken, const ServerListing& serverListing)
{
    nlohmann::json reqBody = nlohmann::json::object();
    reqBody["name"] = serverListing.name;
    reqBody["map"] = serverListing.map;
    reqBody["port"] = serverListing.port;
    reqBody["password"] = serverListing.password;
    reqBody["remote_checksum"] = serverListing.checksum;
    reqBody["version"] = GetVersionString();
    reqBody["gamemode"] = serverListing.gamemode;

    std::string reqBodyStr = reqBody.dump();

    auto res = m_HttpClient.Post("/servers/add", reqBodyStr.c_str(), reqBodyStr.length(), "application/json");

    if (!res) 
    {
        outMessage = "Failed to reach comp-server";
        outToken = "";
        return false;
    }
     
    nlohmann::json resBody = nlohmann::json::parse(res->body);
    if (resBody["success"].is_boolean() && resBody["success"].get<bool>())
    {
        if (resBody["token"].is_string())
            outToken = resBody["token"].get<std::string>();
        else
            outToken = "";
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

    if (!res) 
    {
        outError = "Failed to reach comp-server";
        outServer = ServerListing{};
        return false;
    }

    nlohmann::json resBody = nlohmann::json::parse(res->body);

    if (res && resBody["success"].is_boolean() && resBody["success"])
    {
        outServer = ServerListing{
            resBody["server"]["name"].get<std::string>(),
            resBody["server"]["map"].get<std::string>(),
            resBody["server"]["ip"].get<std::string>(),
            resBody["server"]["port"].get<std::string>()
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

    return false;
}

