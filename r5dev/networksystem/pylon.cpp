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
// Purpose: returns a vector of hosted servers.
// Input  : &svOutMessage - 
// Output : vector<NetGameServer_t>
//-----------------------------------------------------------------------------
vector<NetGameServer_t> CPylon::GetServerList(string& svOutMessage) const
{
    vector<NetGameServer_t> vslList;

    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["version"] = SDK_VERSION;

    string svRequestBody = jsRequestBody.dump(4);
    string svResponse;

    if (pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending server list request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    CURLINFO status;
    if (!QueryMasterServer(pylon_matchmaking_hostname->GetString(), "/servers", svRequestBody, svResponse, svOutMessage, status))
    {
        return vslList;
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(svResponse);
            if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
            {
                for (auto& obj : jsResultBody["servers"])
                {
                    vslList.push_back(
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
                if (jsResultBody["error"].is_string())
                {
                    svOutMessage = jsResultBody["error"].get<string>();
                }
                else
                {
                    svOutMessage = fmt::format("Unknown error with status: {:d}", status);
                }
            }
        }
        else
        {
            if (status)
            {
                if (!svResponse.empty())
                {
                    nlohmann::json jsResultBody = nlohmann::json::parse(svResponse);

                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = fmt::format("Server list error: {:d}", status);
                    }

                    return vslList;
                }

                svOutMessage = fmt::format("Failed comp-server query: {:d}", status);
                return vslList;
            }

            svOutMessage = fmt::format("Failed to reach comp-server: {:s}", "connection timed-out");
            return vslList;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return vslList;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the server by token string.
// Input  : &slOutServer - 
//			&svOutMessage - 
//			&svToken - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken) const
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["token"] = svToken;

    string svRequestBody = jsRequestBody.dump(4);
    string svResponseBuf;

    const bool bDebugLog = pylon_showdebuginfo->GetBool();

    if (bDebugLog)
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending token connect request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    CURLINFO status;
    if (!QueryMasterServer(pylon_matchmaking_hostname->GetString(), "/server/byToken", svRequestBody, svResponseBuf, svOutMessage, status))
    {
        return false;
    }

    if (bDebugLog)
    {
        DevMsg(eDLL_T::ENGINE, "%s - Comp-server replied with status: '%d'\n", __FUNCTION__, status);
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(svResponseBuf);

            if (bDebugLog)
            {
                string svResultBody = jsResultBody.dump(4);
                DevMsg(eDLL_T::ENGINE, "%s - Comp-server response body:\n%s\n", __FUNCTION__, svResultBody.c_str());
            }

            if (jsResultBody["success"].is_boolean() && jsResultBody["success"])
            {
                slOutServer = NetGameServer_t
                {
                        jsResultBody["server"].value("name",""),
                        jsResultBody["server"].value("description",""),
                        jsResultBody["server"].value("hidden","false") == "true",
                        jsResultBody["server"].value("map",""),
                        jsResultBody["server"].value("playlist",""),
                        jsResultBody["server"].value("ip",""),
                        jsResultBody["server"].value("port", ""),
                        jsResultBody["server"].value("key",""),
                        jsResultBody["server"].value("checksum",""),
                        jsResultBody["server"].value("version", SDK_VERSION),
                        jsResultBody["server"].value("playerCount", ""),
                        jsResultBody["server"].value("maxPlayers", ""),
                        jsResultBody["server"].value("timeStamp", 0),
                        jsResultBody["server"].value("publicRef", ""),
                        jsResultBody["server"].value("cachedId", ""),
                };
                return true;
            }
            else
            {
                if (jsResultBody["error"].is_string())
                {
                    svOutMessage = jsResultBody["error"].get<string>();
                }
                else
                {
                    svOutMessage = fmt::format("Unknown error with status: {:d}", status);
                }

                return false;
            }
        }
        else
        {
            if (status)
            {
                if (!svResponseBuf.empty())
                {
                    nlohmann::json jsResultBody = nlohmann::json::parse(svResponseBuf);

                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = fmt::format("Server not found: {:d}", status);
                    }

                    return false;
                }

                svOutMessage = fmt::format("Failed comp-server query: {:d}", status);
                return false;
            }

            svOutMessage = fmt::format("Failed to reach comp-server: {:s}", "connection timed-out");
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
// Input  : &svOutMessage - 
//			&svOutToken - 
//			&netGameServer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& netGameServer) const
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["name"] = netGameServer.m_svHostName;
    jsRequestBody["description"] = netGameServer.m_svDescription;
    jsRequestBody["hidden"] = netGameServer.m_bHidden;
    jsRequestBody["map"] = netGameServer.m_svHostMap;
    jsRequestBody["playlist"] = netGameServer.m_svPlaylist;
    jsRequestBody["ip"] = netGameServer.m_svIpAddress;
    jsRequestBody["port"] = netGameServer.m_svGamePort;
    jsRequestBody["key"] = netGameServer.m_svEncryptionKey;
    jsRequestBody["checksum"] = netGameServer.m_svRemoteChecksum;
    jsRequestBody["version"] = netGameServer.m_svSDKVersion;
    jsRequestBody["playerCount"] = netGameServer.m_svPlayerCount;
    jsRequestBody["maxPlayers"] = netGameServer.m_svMaxPlayers;
    jsRequestBody["timeStamp"] = netGameServer.m_nTimeStamp;
    jsRequestBody["publicRef"] = netGameServer.m_svPublicRef;
    jsRequestBody["cachedId"] = netGameServer.m_svCachedId;

    string svRequestBody = jsRequestBody.dump(4);
    string svResponseBuf;

    const bool bDebugLog = pylon_showdebuginfo->GetBool();
    if (bDebugLog)
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending post host request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    CURLINFO status;
    if (!QueryMasterServer(pylon_matchmaking_hostname->GetString(), "/servers/add", svRequestBody, svResponseBuf, svOutMessage, status))
    {
        return false;
    }

    if (bDebugLog)
    {
        DevMsg(eDLL_T::ENGINE, "%s - Comp-server replied with status: '%d'\n", __FUNCTION__, status);
    }

    try
    {
        if (status == 200) // STATUS_OK
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(svResponseBuf);

            if (bDebugLog)
            {
                string svResultBody = jsResultBody.dump(4);
                DevMsg(eDLL_T::ENGINE, "%s - Comp-server response body:\n%s\n", __FUNCTION__, svResultBody.c_str());
            }

            if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
            {
                if (jsResultBody["token"].is_string())
                {
                    svOutToken = jsResultBody["token"].get<string>();
                }
                else
                {
                    svOutMessage = fmt::format("Invalid response with status: {:d}", status);
                    svOutToken.clear();
                }

                return true;
            }
            else
            {
                if (jsResultBody["error"].is_string())
                {
                    svOutMessage = jsResultBody["error"].get<string>();
                }
                else
                {
                    svOutMessage = fmt::format("Unknown error with status: {:d}", status);
                }
                return false;
            }
        }
        else
        {
            if (status)
            {
                if (!svResponseBuf.empty())
                {
                    nlohmann::json jsResultBody = nlohmann::json::parse(svResponseBuf);

                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = fmt::format("Server host error: {:d}", status);
                    }

                    svOutToken.clear();
                    return false;
                }

                svOutMessage = fmt::format("Failed comp-server query: {:d}", status);
                svOutToken.clear();
                return false;
            }

            svOutMessage = fmt::format("Failed to reach comp-server: {:s}", "connection timed-out");
            svOutToken.clear();
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

    string svHostToken;
    string svErrorMsg;

    bool result = PostServerHost(svErrorMsg, svHostToken, netGameServer);
    if (!result)
    {
        if (!svErrorMsg.empty() && m_ErrorMsg.compare(svErrorMsg) != NULL)
        {
            m_ErrorMsg = svErrorMsg;
            Error(eDLL_T::SERVER, NO_ERROR, "%s\n", svErrorMsg.c_str());
        }
    }
    else // Attempt to log the token, if there is one.
    {
        if (!svHostToken.empty() && m_Token.compare(svHostToken) != NULL)
        {
            m_Token = svHostToken;
            DevMsg(eDLL_T::SERVER, "Published server with token: %s'%s%s%s'\n",
                g_svReset.c_str(), g_svGreyB.c_str(),
                svHostToken.c_str(), g_svReset.c_str());
        }
    }

    return result;
}
#endif // DEDICATED

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : &svIpAddress - 
//			nNucleusID - 
//			&svOutReason - 
// Output : Returns true if banned, false if not banned.
//-----------------------------------------------------------------------------
bool CPylon::CheckForBan(const string& svIpAddress, const uint64_t nNucleusID, string& svOutReason) const
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["id"] = nNucleusID;
    jsRequestBody["ip"] = svIpAddress;

    string svRequestBody = jsRequestBody.dump(4);
    string svResponseBuf;
    string svOutMessage;
    CURLINFO status;

    const bool bDebugLog = pylon_showdebuginfo->GetBool();

    if (bDebugLog)
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending ban check request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    if (!QueryMasterServer(pylon_matchmaking_hostname->GetString(), "/banlist/isBanned", svRequestBody, svResponseBuf, svOutMessage, status))
    {
        return false;
    }

    if (bDebugLog)
    {
        DevMsg(eDLL_T::ENGINE, "%s - Comp-server replied with status: '%d'\n", __FUNCTION__, status);
    }

    try
    {
        if (status == 200)
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(svResponseBuf);

            if (bDebugLog)
            {
                string svResultBody = jsResultBody.dump(4);
                DevMsg(eDLL_T::ENGINE, "%s - Comp-server response body:\n%s\n", __FUNCTION__, svResultBody.c_str());
            }

            if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
            {
                if (jsResultBody["banned"].is_boolean() && jsResultBody["banned"].get<bool>())
                {
                    svOutReason = jsResultBody.value("reason", "#DISCONNECT_BANNED");
                    return true;
                }
            }
        }
        else
        {
            Error(eDLL_T::ENGINE, NO_ERROR, "%s - Failed to query comp-server: status code = %d\n", __FUNCTION__, status);
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
// Input  : &svHostName - 
//			&svApi - 
//			&svInRequest - 
//          &svResponse - 
//          &svOutMessage - 
//          &outStatus - 
// Output : Returns true if successful, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::QueryMasterServer(const string& svHostName, const string& svApi, const string& svInRequest, 
    string& svOutResponse, string& svOutMessage, CURLINFO& outStatus) const
{
    string svUrl;
    CURLFormatUrl(svUrl, svHostName, svApi);

    curl_slist* sList = nullptr;
    CURL* curl = CURLInitRequest(svUrl, svInRequest, svOutResponse, sList);
    if (!curl)
    {
        return false;
    }

    CURLcode res = CURLSubmitRequest(curl, sList);
    if (!CURLHandleError(curl, res, svOutMessage))
    {
        return false;
    }

    outStatus = CURLRetrieveInfo(curl);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
CPylon* g_pMasterServer(new CPylon());
