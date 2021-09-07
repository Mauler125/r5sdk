// r5net.cpp : Defines the functions for the static library.
//

#include "netpch.h"
#include "r5\r5net.h"

std::string R5Net::Client::GetVersionString()
{
    return "beta 1.6";
}

std::vector<ServerListing> R5Net::Client::GetServersList(std::string& outMessage)
{
    std::vector<ServerListing> list{ };

    nlohmann::json reqBody = nlohmann::json::object();
    reqBody["version"] = GetVersionString();

    std::string reqBodyStr = reqBody.dump();

#ifdef DebugR5Net
    std::cout << " [+R5Net+] Sending GetServerList post now..\n";
#endif 
    
    httplib::Result res = m_HttpClient.Post("/servers", reqBody.dump().c_str(), reqBody.dump().length(), "application/json");

#ifdef DebugR5Net
    std::cout << " [+R5Net+] GetServerList replied with " << res->status << "\n";
#endif 

    if (res && res->status == 200) // STATUS_OK
    {
        nlohmann::json resBody = nlohmann::json::parse(res->body);
        if (resBody["success"].is_boolean() && resBody["success"].get<bool>())
        {
            for (auto obj : resBody["servers"])
            {
                list.push_back(
                    ServerListing{
                        obj.value("name",""),
                        obj.value("map", ""),
                        obj.value("ip", ""),
                        obj.value("port", ""),
                        obj.value("gamemode", ""),
                        obj.value("hidden", "false") == "true",
                        obj.value("remote_checksum", ""),
                        obj.value("version", GetVersionString()),
                        obj.value("encKey", "")
                    }
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
        if (res)
        {
            if (!res->body.empty())
            {
                nlohmann::json resBody = nlohmann::json::parse(res->body);

                if (resBody["err"].is_string())
                    outMessage = resBody["err"].get<std::string>();
                else
                    outMessage = std::string("Failed to reach comp-server ") + std::to_string(res->status);

                return list;
            }

            outMessage = std::string("Failed to reach comp-server ") + std::to_string(res->status);
            return list;
        }

        outMessage = "failed to reach comp-server unknown error code.";
        return list;
    }

    return list;
}

bool R5Net::Client::PostServerHost(std::string& outMessage, std::string& outToken, const ServerListing& serverListing)
{
    nlohmann::json reqBody = nlohmann::json::object();
    reqBody["name"] = serverListing.name;
    reqBody["map"] = serverListing.map;
    reqBody["port"] = serverListing.port;
    reqBody["remote_checksum"] = serverListing.remoteChecksum;
    reqBody["version"] = GetVersionString();
    reqBody["gamemode"] = serverListing.playlist;
    reqBody["encKey"] = serverListing.netchanEncryptionKey;
    reqBody["hidden"] = serverListing.hidden;

    std::string reqBodyStr = reqBody.dump();

    #ifdef DebugR5Net
    std::cout << " [+R5Net+] Sending PostServerHost post now..\n" << reqBodyStr << "\n";
#endif 

    httplib::Result res = m_HttpClient.Post("/servers/add", reqBodyStr.c_str(), reqBodyStr.length(), "application/json");

#ifdef DebugR5Net
    std::cout << " [+R5Net+] PostServerHost replied with " << res->status << "\n";
#endif 

    if (res && res->status == 200) // STATUS_OK
    {
        nlohmann::json resBody = nlohmann::json::parse(res->body);
        if (resBody["success"].is_boolean() && resBody["success"].get<bool>())
        {
            if (resBody["token"].is_string())
                outToken = resBody["token"].get<std::string>();
            else
                outToken = std::string();

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
    else
    {
        if (res)
        {
            if (!res->body.empty())
            {
                nlohmann::json resBody = nlohmann::json::parse(res->body);

                if (resBody["err"].is_string())
                    outMessage = resBody["err"].get<std::string>();
                else
                    outMessage = std::string("Failed to reach comp-server ") + std::to_string(res->status);

                outToken = std::string();
                return false;
            }

            outToken = std::string();
            outMessage = std::string("Failed to reach comp-server ") + std::to_string(res->status);
            return false;
        }

        outToken = std::string();
        outMessage = "failed to reach comp-server unknown error code.";
        return false;
    }

    return false;
}

bool R5Net::Client::GetServerByToken(ServerListing& outServer, std::string& outMessage, const std::string token)
{
    nlohmann::json reqBody = nlohmann::json::object();

    reqBody["token"] = token;

#ifdef DebugR5Net
    std::cout << " [+R5Net+] Sending GetServerByToken post now...\n";
#endif 

    httplib::Result res = m_HttpClient.Post("/server/byToken", reqBody.dump().c_str(), reqBody.dump().length(), "application/json");

#ifdef DebugR5Net
    std::cout << " [+R5Net+] GetServerByToken replied with " << res->status << "\n";
#endif 

    if (res && res->status == 200) // STATUS_OK
    {
        if (!res->body.empty())
        {
            nlohmann::json resBody = nlohmann::json::parse(res->body);

            if (res && resBody["success"].is_boolean() && resBody["success"])
            {
                outServer = ServerListing{
                        resBody["server"].value("name",""),
                        resBody["server"].value("map", ""),
                        resBody["server"].value("ip", ""),
                        resBody["server"].value("port", ""),
                        resBody["server"].value("gamemode", ""),
                        resBody["server"].value("hidden", "false") == "true",
                        resBody["server"].value("remote_checksum", ""),
                        resBody["server"].value("version", GetVersionString()),
                        resBody["server"].value("encKey", "")
                };
                return true;
            }
            else
            {
                if (resBody["err"].is_string())
                    outMessage = resBody["err"].get<std::string>();
                else
                    outMessage = "";

                outServer = ServerListing{};
                return false;
            }
        }
    }
    else
    {
        if (res)
        {
            if (!res->body.empty())
            {
                nlohmann::json resBody = nlohmann::json::parse(res->body);

                if (resBody["err"].is_string())
                    outMessage = resBody["err"].get<std::string>();
                else
                    outMessage = std::string("Failed to reach comp-server ") + std::to_string(res->status);

                return false;
            }

            outMessage = std::string("Failed to reach comp-server ") + std::to_string(res->status);
            return false;
        }

        outMessage = "failed to reach comp-server unknown error code.";
        outServer = ServerListing{};
        return false;
    }

    return false;
}

bool R5Net::Client::GetClientIsBanned(const std::string ip, std::int64_t orid, std::string& outErrCl)
{
    nlohmann::json reqBody = nlohmann::json::object();
    reqBody["ip"] = ip;
    reqBody["orid"] = orid;

    httplib::Result res = m_HttpClient.Post("/banlist/isBanned", reqBody.dump().c_str(), reqBody.dump().length(), "application/json");

    if (res && res->status == 200)
    {
        nlohmann::json resBody = nlohmann::json::parse(res->body);

        if (resBody["success"].is_boolean() && resBody["success"].get<bool>())
        {
            if (resBody["isBanned"].is_boolean() && resBody["isBanned"].get<bool>())
            {
                outErrCl = resBody.value("errCl", "Generic error (code:gen). Contact R5Reloaded developers.");
                return true;
            }
        }
    }
    return false;
}

