// r5net.cpp : Defines the functions for the static library.
//

#include "core/stdafx.h"
#include "tier0/basetypes.h"
#include "tier0/cvar.h"
#include "engine/sys_utils.h"
#include "networksystem/r5net.h"

//-----------------------------------------------------------------------------
// Purpose: returns the sdk version string.
//-----------------------------------------------------------------------------
std::string R5Net::Client::GetSDKVersion()
{
    return SDK_VERSION;
}

//-----------------------------------------------------------------------------
// Purpose: returns a vector of hosted servers.
//-----------------------------------------------------------------------------
std::vector<ServerListing> R5Net::Client::GetServersList(std::string& svOutMessage)
{
    std::vector<ServerListing> vslList{};

    nlohmann::json jsReqBody = nlohmann::json::object();
    jsReqBody["version"] = GetSDKVersion();

    std::string reqBodyStr = jsReqBody.dump();

    if (r5net_show_debug->m_pParent->m_iValue > 0)
    {
       DevMsg(eDLL_T::ENGINE, "Sending GetServerList post.\n");
    }

    httplib::Result htResults = m_HttpClient.Post("/servers", jsReqBody.dump().c_str(), jsReqBody.dump().length(), "application/json");

    if (r5net_show_debug->m_pParent->m_iValue > 0)
    {
        DevMsg(eDLL_T::ENGINE, "GetServerList replied with '%d'.\n", htResults->status);
    }

    if (htResults && htResults->status == 200) // STATUS_OK
    {
        nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);
        if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
        {
            for (auto &obj : jsResultBody["servers"])
            {
                vslList.push_back(
                    ServerListing{
                        obj.value("name",""),
                        obj.value("map", ""),
                        obj.value("ip", ""),
                        obj.value("port", ""),
                        obj.value("gamemode", ""),
                        obj.value("hidden", "false") == "true",
                        obj.value("remote_checksum", ""),
                        obj.value("version", GetSDKVersion()),
                        obj.value("encKey", "")
                    }
                );
            }
        }
        else
        {
            if (jsResultBody["err"].is_string())
            {
                svOutMessage = jsResultBody["err"].get<std::string>();
            }
            else
            {
                svOutMessage = "An unknown error occured!";
            }
        }
    }
    else
    {
        if (htResults)
        {
            if (!htResults->body.empty())
            {
                nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);

                if (jsResultBody["err"].is_string())
                {
                    svOutMessage = jsResultBody["err"].get<std::string>();
                }
                else
                {
                    svOutMessage = std::string("Failed to reach comp-server ") + std::to_string(htResults->status);
                }

                return vslList;
            }

            svOutMessage = std::string("Failed to reach comp-server ") + std::to_string(htResults->status);
            return vslList;
        }

        svOutMessage = "Failed to reach comp-server. Unknown error code.";
        return vslList;
    }

    return vslList;
}

//-----------------------------------------------------------------------------
// Purpose: Sends host server POST request.
// Input  : &svOutMessage - 
//			&svOutToken - 
//			&slServerListing - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool R5Net::Client::PostServerHost(std::string& svOutMessage, std::string& svOutToken, const ServerListing& slServerListing)
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["name"] = slServerListing.svServerName;
    jsRequestBody["map"] = slServerListing.svMapName;
    jsRequestBody["port"] = slServerListing.svPort;
    jsRequestBody["remote_checksum"] = slServerListing.svRemoteChecksum;
    jsRequestBody["version"] = GetSDKVersion();
    jsRequestBody["gamemode"] = slServerListing.svPlaylist;
    jsRequestBody["encKey"] = slServerListing.svEncryptionKey;
    jsRequestBody["hidden"] = slServerListing.bHidden;

    std::string svRequestBody = jsRequestBody.dump();

    if (r5net_show_debug->m_pParent->m_iValue > 0)
    {
        DevMsg(eDLL_T::ENGINE, "Sending PostServerHost post '%s'.\n", svRequestBody.c_str());
    }

    httplib::Result htResults = m_HttpClient.Post("/servers/add", svRequestBody.c_str(), svRequestBody.length(), "application/json");

    if (r5net_show_debug->m_pParent->m_iValue > 0)
    {
        DevMsg(eDLL_T::ENGINE, "PostServerHost replied with '%d'.\n", htResults->status);
    }

    if (htResults && htResults->status == 200) // STATUS_OK
    {
        nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);
        if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
        {
            if (jsResultBody["token"].is_string())
            {
                svOutToken = jsResultBody["token"].get<std::string>();
            }
            else
            {
                svOutToken = std::string();
            }

            return true;
        }
        else
        {
            if (jsResultBody["err"].is_string())
            {
                svOutMessage = jsResultBody["err"].get<std::string>();
            }
            else
            {
                svOutMessage = "An unknown error occured!";
            }
            return false;
        }
    }
    else
    {
        if (htResults)
        {
            if (!htResults->body.empty())
            {
                nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);

                if (jsResultBody["err"].is_string())
                {
                    svOutMessage = jsResultBody["err"].get<std::string>();
                }
                else
                {
                    svOutMessage = std::string("Failed to reach comp-server ") + std::to_string(htResults->status);
                }

                svOutToken = std::string();
                return false;
            }

            svOutToken = std::string();
            svOutMessage = std::string("Failed to reach comp-server ") + std::to_string(htResults->status);
            return false;
        }

        svOutToken = std::string();
        svOutMessage = "Failed to reach comp-server. Unknown error code.";
        return false;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the server by token string.
// Input  : &slOutServer - 
//			&svOutMessage - 
//			svToken - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool R5Net::Client::GetServerByToken(ServerListing& slOutServer, std::string& svOutMessage, const std::string svToken)
{
    nlohmann::json jsRequestBody = nlohmann::json::object();

    jsRequestBody["token"] = svToken;

    if (r5net_show_debug->m_pParent->m_iValue > 0)
    {
        DevMsg(eDLL_T::ENGINE, "Sending GetServerByToken post.\n");
    }

    httplib::Result htResults = m_HttpClient.Post("/server/byToken", jsRequestBody.dump().c_str(), jsRequestBody.dump().length(), "application/json");

    if (r5net_show_debug->m_pParent->m_iValue > 0)
    {
        DevMsg(eDLL_T::ENGINE, "GetServerByToken replied with '%d'\n", htResults->status);
    }

    if (htResults && htResults->status == 200) // STATUS_OK
    {
        if (!htResults->body.empty())
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);

            if (htResults && jsResultBody["success"].is_boolean() && jsResultBody["success"])
            {
                slOutServer = ServerListing{
                        jsResultBody["server"].value("name",""),
                        jsResultBody["server"].value("map", ""),
                        jsResultBody["server"].value("ip", ""),
                        jsResultBody["server"].value("port", ""),
                        jsResultBody["server"].value("gamemode", ""),
                        jsResultBody["server"].value("hidden", "false") == "true",
                        jsResultBody["server"].value("remote_checksum", ""),
                        jsResultBody["server"].value("version", GetSDKVersion()),
                        jsResultBody["server"].value("encKey", "")
                };
                return true;
            }
            else
            {
                if (jsResultBody["err"].is_string())
                {
                    svOutMessage = jsResultBody["err"].get<std::string>();
                }
                else
                {
                    svOutMessage = "";
                }

                slOutServer = ServerListing{};
                return false;
            }
        }
    }
    else
    {
        if (htResults)
        {
            if (!htResults->body.empty())
            {
                nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);

                if (jsResultBody["err"].is_string())
                {
                    svOutMessage = jsResultBody["err"].get<std::string>();
                }
                else
                {
                    svOutMessage = std::string("Failed to reach comp-server ") + std::to_string(htResults->status);
                }

                return false;
            }

            svOutMessage = std::string("Failed to reach comp-server ") + std::to_string(htResults->status);
            return false;
        }

        svOutMessage = "Failed to reach comp-server. Unknown error code.";
        slOutServer = ServerListing{};
        return false;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : svIpAddress - 
//			nOriginID - 
//			&svOutErrCl - 
// Output : Returns true if banned, false if not banned.
//-----------------------------------------------------------------------------
bool R5Net::Client::GetClientIsBanned(const std::string svIpAddress, std::int64_t nOriginID, std::string& svOutErrCl)
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["ip"] = svIpAddress;
    jsRequestBody["orid"] = nOriginID;

    httplib::Result htResults = m_HttpClient.Post("/banlist/isBanned", jsRequestBody.dump().c_str(), jsRequestBody.dump().length(), "application/json");

    if (htResults && htResults->status == 200)
    {
        nlohmann::json jsResultBody = nlohmann::json::parse(htResults->body);

        if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
        {
            if (jsResultBody["isBanned"].is_boolean() && jsResultBody["isBanned"].get<bool>())
            {
                svOutErrCl = jsResultBody.value("errCl", "Generic error (code:gen). Contact R5Reloaded developers.");
                return true;
            }
        }
    }
    return false;
}
///////////////////////////////////////////////////////////////////////////////
R5Net::Client* g_pR5net(new R5Net::Client("r5a-comp-sv.herokuapp.com"));
