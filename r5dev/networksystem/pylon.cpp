//=====================================================================================//
//
// Purpose: Implementation of the pylon server backend.
//
// $NoKeywords: $
//=====================================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <tier2/curlutils.h>
#include <networksystem/pylon.h>
#ifndef CLIENT_DLL
#include <engine/server/server.h>
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: gets a vector of hosted servers.
// Input  : &outMessage - 
// Output : vector<NetGameServer_t>
//-----------------------------------------------------------------------------
vector<NetGameServer_t> CPylon::GetServerList(string& outMessage) const
{
    vector<NetGameServer_t> vecServers;

    nlohmann::json requestJson = nlohmann::json::object();
    requestJson["version"] = SDK_VERSION;

    string responseBody;
    CURLINFO status;

    if (!QueryAPI("/servers", requestJson.dump(4), responseBody, outMessage, status))
    {
        return vecServers;
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            nlohmann::json resultJson = nlohmann::json::parse(responseBody);
            if (resultJson["success"].is_boolean() && resultJson["success"].get<bool>())
            {
                for (auto& obj : resultJson["servers"])
                {
                    vecServers.push_back(
                        NetGameServer_t
                        {
                            obj.value("name",""),
                            obj.value("description",""),
                            obj.value("hidden","false") == "true",
                            obj.value("map",""),
                            obj.value("playlist",""),
                            obj.value("ip",""),
                            obj.value("port", ""),
                            obj.value("key",""),
                            obj.value("checksum",""),
                            obj.value("version", SDK_VERSION),
                            obj.value("playerCount", ""),
                            obj.value("maxPlayers", ""),
                            obj.value("timeStamp", 0),
                            obj.value("publicRef", ""),
                            obj.value("cachedId", ""),
                        }
                    );
                }
            }
            else
            {
                ExtractError(resultJson, outMessage, status);
            }
        }
        else
        {
            ExtractError(responseBody, outMessage, status, "Server list error");
            return vecServers;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return vecServers;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the server by token string.
// Input  : &outGameServer - 
//			&outMessage - 
//			&token - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetServerByToken(NetGameServer_t& outGameServer,
    string& outMessage, const string& token) const
{
    nlohmann::json requestJson = nlohmann::json::object();
    requestJson["token"] = token;

    string responseBody;
    CURLINFO status;

    if (!QueryAPI("/server/byToken", requestJson.dump(4), responseBody, outMessage, status))
    {
        return false;
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            nlohmann::json responseJson = nlohmann::json::parse(responseBody);

            if (pylon_showdebuginfo->GetBool())
            {
                responseBody = responseJson.dump(4);
                DevMsg(eDLL_T::ENGINE, "%s: Response body:\n%s\n",
                    __FUNCTION__, responseBody.c_str());
            }

            if (responseJson["success"].is_boolean() && responseJson["success"])
            {
                outGameServer = NetGameServer_t
                {
                        responseJson["server"].value("name",""),
                        responseJson["server"].value("description",""),
                        responseJson["server"].value("hidden","false") == "true",
                        responseJson["server"].value("map",""),
                        responseJson["server"].value("playlist",""),
                        responseJson["server"].value("ip",""),
                        responseJson["server"].value("port", ""),
                        responseJson["server"].value("key",""),
                        responseJson["server"].value("checksum",""),
                        responseJson["server"].value("version", SDK_VERSION),
                        responseJson["server"].value("playerCount", ""),
                        responseJson["server"].value("maxPlayers", ""),
                        responseJson["server"].value("timeStamp", 0),
                        responseJson["server"].value("publicRef", ""),
                        responseJson["server"].value("cachedId", ""),
                };
                return true;
            }
            else
            {
                ExtractError(responseJson, outMessage, status);
                return false;
            }
        }
        else
        {
            ExtractError(responseBody, outMessage, status, "Server not found");
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sends host server POST request.
// Input  : &outMessage - 
//			&outToken - 
//			&netGameServer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::PostServerHost(string& outMessage, string& outToken, const NetGameServer_t& netGameServer) const
{
    nlohmann::json requestJson = nlohmann::json::object();
    requestJson["name"] = netGameServer.m_svHostName;
    requestJson["description"] = netGameServer.m_svDescription;
    requestJson["hidden"] = netGameServer.m_bHidden;
    requestJson["map"] = netGameServer.m_svHostMap;
    requestJson["playlist"] = netGameServer.m_svPlaylist;
    requestJson["ip"] = netGameServer.m_svIpAddress;
    requestJson["port"] = netGameServer.m_svGamePort;
    requestJson["key"] = netGameServer.m_svEncryptionKey;
    requestJson["checksum"] = netGameServer.m_svRemoteChecksum;
    requestJson["version"] = netGameServer.m_svSDKVersion;
    requestJson["playerCount"] = netGameServer.m_svPlayerCount;
    requestJson["maxPlayers"] = netGameServer.m_svMaxPlayers;
    requestJson["timeStamp"] = netGameServer.m_nTimeStamp;
    requestJson["publicRef"] = netGameServer.m_svPublicRef;
    requestJson["cachedId"] = netGameServer.m_svCachedId;

    string responseBody;
    CURLINFO status;

    if (!QueryAPI("/servers/add", requestJson.dump(4), responseBody, outMessage, status))
    {
        return false;
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            nlohmann::json responseJson = nlohmann::json::parse(responseBody);

            if (pylon_showdebuginfo->GetBool())
            {
                responseBody = responseJson.dump(4);
                DevMsg(eDLL_T::ENGINE, "%s: Response body:\n%s\n",
                    __FUNCTION__, responseBody.c_str());
            }

            if (responseJson["success"].is_boolean() && responseJson["success"].get<bool>())
            {
                if (responseJson["token"].is_string())
                {
                    outToken = responseJson["token"].get<string>();
                }
                else
                {
                    outMessage = Format("Invalid response with status: %d", static_cast<int>(status));
                    outToken.clear();
                }

                return true;
            }
            else
            {
                ExtractError(responseJson, outMessage, status);
                outToken.clear();

                return false;
            }
        }
        else
        {
            ExtractError(responseBody, outMessage, status, "Server host error");
            outToken.clear();

            return false;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return false;
}

#ifdef DEDICATED
//-----------------------------------------------------------------------------
// Purpose: Send keep alive request to Pylon Master Server.
// Input  : &netGameServer - 
// Output : Returns true on success, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::KeepAlive(const NetGameServer_t& netGameServer)
{
    if (!g_pServer->IsActive() || !sv_pylonVisibility->GetBool()) // Check for active game.
    {
        return false;
    }

    string errorMsg;
    string hostToken;

    const bool result = PostServerHost(errorMsg, hostToken, netGameServer);
    if (!result)
    {
        if (!errorMsg.empty() && m_ErrorMsg.compare(errorMsg) != NULL)
        {
            m_ErrorMsg = errorMsg;
            Error(eDLL_T::SERVER, NO_ERROR, "%s\n", errorMsg.c_str());
        }
    }
    else // Attempt to log the token, if there is one.
    {
        if (!hostToken.empty() && m_Token.compare(hostToken) != NULL)
        {
            m_Token = hostToken;
            DevMsg(eDLL_T::SERVER, "Published server with token: %s'%s%s%s'\n",
                g_svReset.c_str(), g_svGreyB.c_str(),
                hostToken.c_str(), g_svReset.c_str());
        }
    }

    return result;
}
#endif // DEDICATED

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : &ipAddress - 
//			nucleusId  - 
//			&outReason - <- contains banned reason if any.
// Output : Returns true if banned, false if not banned.
//-----------------------------------------------------------------------------
bool CPylon::CheckForBan(const string& ipAddress, const uint64_t nucleusId, string& outReason) const
{
    nlohmann::json requestJson = nlohmann::json::object();
    requestJson["id"] = nucleusId;
    requestJson["ip"] = ipAddress;

    string responseBody;
    string outMessage;
    CURLINFO status;

    if (!QueryAPI("/banlist/isBanned", requestJson.dump(4), responseBody, outMessage, status))
    {
        return false;
    }

    if (status != 200)
    {
        Error(eDLL_T::ENGINE, NO_ERROR, "%s - Failed to query comp-server: status code = %d\n", __FUNCTION__, status);
        return false;
    }

    try
    {
        nlohmann::json responseJson = nlohmann::json::parse(responseBody);

        if (pylon_showdebuginfo->GetBool())
        {
            responseBody = responseJson.dump(4);
            DevMsg(eDLL_T::ENGINE, "%s: Response body:\n%s\n",
                __FUNCTION__, responseBody.c_str());
        }

        if (responseJson["success"].is_boolean() && responseJson["success"].get<bool>())
        {
            if (responseJson["banned"].is_boolean() && responseJson["banned"].get<bool>())
            {
                outReason = responseJson.value("reason", "#DISCONNECT_BANNED");
                return true;
            }
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sends query to master server.
// Input  : &apiName     - 
//          &request     - 
//          &outResponse - 
//          &outMessage  - <- contains an error message if any.
//          &outStatus   - 
// Output : Returns true if successful, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::QueryAPI(const string& apiName, const string& request,
    string& outResponse, string& outMessage, CURLINFO& outStatus) const
{
    const bool showDebug = pylon_showdebuginfo->GetBool();
    const char* hostName = pylon_matchmaking_hostname->GetString();

    if (showDebug)
    {
        DevMsg(eDLL_T::ENGINE, "%s: Sending '%s' request to '%s':\n%s\n",
            __FUNCTION__, apiName.c_str(), hostName, request.c_str());
    }

    if (!QueryMasterServer(pylon_matchmaking_hostname->GetString(), apiName,
        request, outResponse, outMessage, outStatus))
    {
        return false;
    }

    if (showDebug)
    {
        DevMsg(eDLL_T::ENGINE, "%s: Host '%s' replied with status: '%d'\n",
            __FUNCTION__, hostName, outStatus);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sends query to master server.
// Input  : &hostName    - 
//          &apiName     - 
//          &request     - 
//          &outResponse - 
//          &outMessage  - <- contains an error message if any.
//          &outStatus   - 
// Output : Returns true if successful, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::QueryMasterServer(const string& hostName, const string& apiName,
    const string& request,  string& outResponse, string& outMessage,
    CURLINFO& outStatus) const
{
    string finalUrl;
    CURLFormatUrl(finalUrl, hostName, apiName);

    curl_slist* sList = nullptr;
    CURL* curl = CURLInitRequest(finalUrl, request, outResponse, sList);
    if (!curl)
    {
        return false;
    }

    CURLcode res = CURLSubmitRequest(curl, sList);
    if (!CURLHandleError(curl, res, outMessage))
    {
        return false;
    }

    outStatus = CURLRetrieveInfo(curl);
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the error from the result json.
// Input  : &resultJson - 
//          &outMessage - 
//          status      - 
//          *errorText  - 
//-----------------------------------------------------------------------------
void CPylon::ExtractError(const nlohmann::json& resultJson, string& outMessage,
    CURLINFO status, const char* errorText) const
{
    if (resultJson["error"].is_string())
    {
        outMessage = resultJson["error"].get<string>();
    }
    else
    {
        if (!errorText)
        {
            errorText = "Unknown error with status";
        }

        outMessage = Format("%s: %d", errorText, static_cast<int>(status));
    }
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the error from the response buffer.
// Input  : &response   - 
//          &outMessage - 
//          status      - 
//          *errorText  - 
//-----------------------------------------------------------------------------
void CPylon::ExtractError(const string& response, string& outMessage,
    CURLINFO status, const char* errorText) const
{
    if (!response.empty())
    {
        nlohmann::json resultBody = nlohmann::json::parse(response);
        ExtractError(resultBody, outMessage, status, errorText);
    }
    else if (status)
    {
        outMessage = Format("Failed comp-server query: %d", static_cast<int>(status));
    }
    else
    {
        outMessage = Format("Failed to reach comp-server: %s", "connection timed-out");
    }
}

///////////////////////////////////////////////////////////////////////////////
CPylon* g_pMasterServer(new CPylon());
