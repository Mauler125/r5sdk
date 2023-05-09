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
#include <engine/server/server.h>

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

    nlohmann::json responseJson;
    CURLINFO status;

    if (!SendRequest("/servers", requestJson, responseJson,
        outMessage, status, "server list error"))
    {
        return vecServers;
    }

    if (!responseJson.contains("servers"))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return vecServers;
    }

    try
    {
        for (auto& obj : responseJson["servers"])
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
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - %s\n", __FUNCTION__, ex.what());
        vecServers.clear(); // Clear as the vector may be partially filled.
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

    nlohmann::json responseJson;
    CURLINFO status;

    if (!SendRequest("/server/byToken", requestJson, responseJson,
        outMessage, status, "server not found"))
    {
        return false;
    }

    if (!responseJson.contains("server"))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    try
    {
        nlohmann::json& serverJson = responseJson["server"];
        outGameServer = NetGameServer_t
        {
                serverJson.value("name",""),
                serverJson.value("description",""),
                serverJson.value("hidden","false") == "true",
                serverJson.value("map",""),
                serverJson.value("playlist",""),
                serverJson.value("ip",""),
                serverJson.value("port", ""),
                serverJson.value("key",""),
                serverJson.value("checksum",""),
                serverJson.value("version", SDK_VERSION),
                serverJson.value("playerCount", ""),
                serverJson.value("maxPlayers", ""),
                serverJson.value("timeStamp", 0),
                serverJson.value("publicRef", ""),
                serverJson.value("cachedId", ""),
        };

        return true;
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - %s\n", __FUNCTION__, ex.what());
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
bool CPylon::PostServerHost(string& outMessage, string& outToken,
    const NetGameServer_t& netGameServer) const
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

    nlohmann::json responseJson;
    CURLINFO status;

    if (!SendRequest("/servers/add", requestJson, responseJson,
        outMessage, status, "server host error"))
    {
        return false;
    }

    if (netGameServer.m_bHidden)
    {
        nlohmann::json& tokenJson = responseJson["token"];

        if (!tokenJson.is_string())
        {
            outMessage = Format("Invalid response with status: %d", int(status));
            outToken.clear();

            return false;
        }

        outToken = tokenJson.get<string>();
    }

    return true;
}

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
                g_svReset, g_svGreyB,
                hostToken, g_svReset);
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// Purpose: Checks a list of clients for their banned status.
// Input  : &inBannedVec - 
//			&outBannedVec  - 
// Output : True on success, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::GetBannedList(const BannedVec_t& inBannedVec, BannedVec_t& outBannedVec) const
{
    nlohmann::json arrayJson = nlohmann::json::array();

    for (const auto& bannedPair : inBannedVec)
    {
        nlohmann::json player;
        player["id"] = bannedPair.second;
        player["ip"] = bannedPair.first;
        arrayJson.push_back(player);
    }

    nlohmann::json playerArray;
    playerArray["players"] = arrayJson;

    string outMessage;
    CURLINFO status;

    if (!SendRequest("/banlist/bulkCheck", playerArray,
        arrayJson, outMessage, status, "banned bulk check error"))
    {
        return false;
    }

    if (!arrayJson.contains("bannedPlayers"))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    try
    {
        for (auto& obj : arrayJson["bannedPlayers"])
        {
            outBannedVec.push_back(
                std::make_pair(
                    obj.value("reason", "#DISCONNECT_BANNED"),
                    obj.value("id", uint64_t(0))
                )
            );
        }

        return true;
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - %s\n", __FUNCTION__, ex.what());
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : &ipAddress - 
//			nucleusId  - 
//			&outReason - <- contains banned reason if any.
// Output : True if banned, false if not banned.
//-----------------------------------------------------------------------------
bool CPylon::CheckForBan(const string& ipAddress, const uint64_t nucleusId,
    const string& personaName, string& outReason) const
{
    nlohmann::json requestJson = nlohmann::json::object();
    requestJson["name"] = personaName;
    requestJson["id"] = nucleusId;
    requestJson["ip"] = ipAddress;

    nlohmann::json responseJson;
    string outMessage;
    CURLINFO status;

    if (!SendRequest("/banlist/isBanned", requestJson,
        responseJson, outMessage, status, "banned check error"))
    {
        return false;
    }

    try
    {
        if (responseJson["banned"].is_boolean() &&
            responseJson["banned"].get<bool>())
        {
            outReason = responseJson.value("reason", "#DISCONNECT_BANNED");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - %s\n", __FUNCTION__, ex.what());
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sends request to Pylon Master Server.
// Input  : *endpoint -
//			&requestJson -
//			&responseJson -
//			&outMessage -
//			&status -
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::SendRequest(const char* endpoint, const nlohmann::json& requestJson,
    nlohmann::json& responseJson, string& outMessage, CURLINFO& status, const char* errorText) const
{
    string responseBody;

    if (!QueryServer(endpoint, requestJson.dump(4).c_str(), responseBody, outMessage, status))
    {
        return false;
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            responseJson = nlohmann::json::parse(responseBody);
            LogBody(responseJson);

            if (responseJson["success"].is_boolean() &&
                responseJson["success"].get<bool>())
            {
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
            ExtractError(responseBody, outMessage, status, errorText);
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing response:\n%s\n", __FUNCTION__, ex.what());
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Sends query to master server.
// Input  : *endpoint    - 
//          *request     - 
//          &outResponse - 
//          &outMessage  - <- contains an error message on failure.
//          &outStatus   - 
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::QueryServer(const char* endpoint, const char* request,
    string& outResponse, string& outMessage, CURLINFO& outStatus) const
{
    const bool showDebug = pylon_showdebuginfo->GetBool();
    const char* hostName = pylon_matchmaking_hostname->GetString();

    if (showDebug)
    {
        DevMsg(eDLL_T::ENGINE, "Sending request to '%s' with endpoint '%s':\n%s\n",
            hostName, endpoint, request);
    }

    string finalUrl;
    CURLFormatUrl(finalUrl, hostName, endpoint);

    curl_slist* sList = nullptr;
    CURL* curl = CURLInitRequest(finalUrl.c_str(), request, outResponse, sList);
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

    if (showDebug)
    {
        DevMsg(eDLL_T::ENGINE, "Host '%s' replied with status: '%d'\n",
            hostName, outStatus);
    }

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
            errorText = "unknown error";
        }

        outMessage = Format("Failed with status: %d (%s)",
            int(status), errorText);
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
        outMessage = Format("Failed server query: %d", int(status));
    }
    else
    {
        outMessage = Format("Failed to reach server: %s",
            "connection timed out");
    }
}

//-----------------------------------------------------------------------------
// Purpose: Logs the response body if debug is enabled.
// Input  : &responseJson -
//-----------------------------------------------------------------------------
void CPylon::LogBody(const nlohmann::json& responseJson) const
{
    if (pylon_showdebuginfo->GetBool())
    {
        const string responseBody = responseJson.dump(4);
        DevMsg(eDLL_T::ENGINE, "\n%s\n", responseBody.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
CPylon* g_pMasterServer(new CPylon());
