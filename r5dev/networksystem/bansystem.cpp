//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#include "engine/server/server.h"
#include "engine/client/client.h"
#include "filesystem/filesystem.h"
#include "networksystem/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banned list
//-----------------------------------------------------------------------------
void CBanSystem::Load(void)
{
	if (IsBanListValid())
		m_vBanList.clear();

	FileHandle_t pFile = FileSystem()->Open("banlist.json", "rt");
	if (!pFile)
		return;

	const ssize_t nLen = FileSystem()->Size(pFile);
	std::unique_ptr<char[]> pBuf(new char[nLen + 1]);

	const ssize_t nRead = FileSystem()->Read(pBuf.get(), nLen, pFile);
	FileSystem()->Close(pFile);

	pBuf.get()[nRead] = '\0'; // Null terminate the string buffer containing our banned list.

	try
	{
		nlohmann::json jsIn = nlohmann::json::parse(pBuf.get());

		size_t nTotalBans = 0;
		if (!jsIn.is_null())
		{
			if (!jsIn["totalBans"].is_null())
				nTotalBans = jsIn["totalBans"].get<size_t>();
		}

		for (size_t i = 0; i < nTotalBans; i++)
		{
			nlohmann::json jsEntry = jsIn[std::to_string(i)];
			if (!jsEntry.is_null())
			{
				const string  svIpAddress = jsEntry["ipAddress"].get<string>();
				const uint64_t nNucleusID = jsEntry["nucleusId"].get<uint64_t>();

				m_vBanList.push_back(std::make_pair(svIpAddress, nNucleusID));
			}
		}
	}
	catch (const std::exception& ex)
	{
		Warning(eDLL_T::SERVER, "%s: Exception while parsing banned list:\n%s\n", __FUNCTION__, ex.what());
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves the banned list
//-----------------------------------------------------------------------------
void CBanSystem::Save(void) const
{
	FileHandle_t pFile = FileSystem()->Open("banlist.json", "wt", "PLATFORM");
	if (!pFile)
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, "banlist.json");
		return;
	}

	try
	{
		nlohmann::json jsOut;
		for (size_t i = 0; i < m_vBanList.size(); i++)
		{
			jsOut[std::to_string(i)]["ipAddress"] = m_vBanList[i].first;
			jsOut[std::to_string(i)]["nucleusId"] = m_vBanList[i].second;
		}

		jsOut["totalBans"] = m_vBanList.size();
		string svJsOut = jsOut.dump(4);

		FileSystem()->Write(svJsOut.data(), svJsOut.size(), pFile);
	}
	catch (const std::exception& ex)
	{
		Warning(eDLL_T::SERVER, "%s: Exception while parsing banned list:\n%s\n", __FUNCTION__, ex.what());
	}

	FileSystem()->Close(pFile);
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banned list
// Input  : *ipAddress - 
//			nucleusId - 
//-----------------------------------------------------------------------------
bool CBanSystem::AddEntry(const char* ipAddress, const uint64_t nucleusId)
{
	Assert(VALID_CHARSTAR(ipAddress));
	const auto idPair = std::make_pair(string(ipAddress), nucleusId);

	if (IsBanListValid())
	{
		auto it = std::find(m_vBanList.begin(), m_vBanList.end(), idPair);

		if (it == m_vBanList.end())
		{
			m_vBanList.push_back(idPair);
			return true;
		}
	}
	else
	{
		m_vBanList.push_back(idPair);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the banned list
// Input  : *ipAddress - 
//			nucleusId - 
//-----------------------------------------------------------------------------
bool CBanSystem::DeleteEntry(const char* ipAddress, const uint64_t nucleusId)
{
	Assert(VALID_CHARSTAR(ipAddress));

	if (IsBanListValid())
	{
		auto it = std::find_if(m_vBanList.begin(), m_vBanList.end(),
			[&](const pair<const string, const uint64_t>& element)
			{
				return (strcmp(ipAddress, element.first.c_str()) == NULL
				|| element.second == nucleusId);
			});

		if (it != m_vBanList.end())
		{
			m_vBanList.erase(it);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if specified ip address or nucleus id is banned
// Input  : *ipAddress - 
//			nucleusId - 
// Output : true if banned, false if not banned
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanned(const char* ipAddress, const uint64_t nucleusId) const
{
	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		const string& bannedIpAddress = m_vBanList[i].first;
		const uint64_t bannedNucleusID = m_vBanList[i].second;

		if (bannedIpAddress.empty()
			|| !bannedNucleusID) // Cannot be null.
		{
			continue;
		}

		if (bannedIpAddress.compare(ipAddress) == NULL
			|| nucleusId == bannedNucleusID)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if banned list is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanListValid(void) const
{
	return !m_vBanList.empty();
}

//-----------------------------------------------------------------------------
// Purpose: kicks a player by given name
// Input  : *playerName - 
//			*reason - 
//-----------------------------------------------------------------------------
void CBanSystem::KickPlayerByName(const char* playerName, const char* reason)
{
	if (!VALID_CHARSTAR(playerName))
		return;

	AuthorPlayerByName(playerName, false, reason);
}

//-----------------------------------------------------------------------------
// Purpose: kicks a player by given handle or id
// Input  : *playerHandle - 
//			*reason - 
//-----------------------------------------------------------------------------
void CBanSystem::KickPlayerById(const char* playerHandle, const char* reason)
{
	if (!VALID_CHARSTAR(playerHandle))
		return;

	AuthorPlayerById(playerHandle, false, reason);
}

//-----------------------------------------------------------------------------
// Purpose: bans a player by given name
// Input  : *playerName - 
//			*reason - 
//-----------------------------------------------------------------------------
void CBanSystem::BanPlayerByName(const char* playerName, const char* reason)
{
	if (!VALID_CHARSTAR(playerName))
		return;

	AuthorPlayerByName(playerName, true, reason);
}

//-----------------------------------------------------------------------------
// Purpose: bans a player by given handle or id
// Input  : *playerHandle - 
//			*reason - 
//-----------------------------------------------------------------------------
void CBanSystem::BanPlayerById(const char* playerHandle, const char* reason)
{
	if (!VALID_CHARSTAR(playerHandle))
		return;

	AuthorPlayerById(playerHandle, true, reason);
}

//-----------------------------------------------------------------------------
// Purpose: unbans a player by given nucleus id or ip address
// Input  : *criteria - 
//-----------------------------------------------------------------------------
void CBanSystem::UnbanPlayer(const char* criteria)
{
	try
	{
		bool bSave = false;
		if (StringIsDigit(criteria)) // Check if we have an ip address or nucleus id.
		{
			if (DeleteEntry("<<invalid>>", std::stoll(criteria))) // Delete ban entry.
			{
				bSave = true;
			}
		}
		else
		{
			if (DeleteEntry(criteria, 0)) // Delete ban entry.
			{
				bSave = true;
			}
		}

		if (bSave)
		{
			Save(); // Save modified vector to file.
			DevMsg(eDLL_T::SERVER, "Removed '%s' from banned list\n", criteria);
		}
	}
	catch (const std::exception& e)
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - %s\n", __FUNCTION__, e.what());
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: authors player by given name
// Input  : *playerName - 
//			shouldBan   - (only kicks if false)
//			*reason - 
//-----------------------------------------------------------------------------
void CBanSystem::AuthorPlayerByName(const char* playerName, const bool shouldBan, const char* reason)
{
	Assert(VALID_CHARSTAR(playerName));
	bool bDisconnect = false;
	bool bSave = false;

	if (!reason)
		reason = shouldBan ? "Banned from server" : "Kicked from server";

	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pServer->GetClient(i);
		if (!pClient)
			continue;

		CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		if (strlen(pNetChan->GetName()) > 0)
		{
			if (strcmp(playerName, pNetChan->GetName()) == NULL) // Our wanted name?
			{
				if (shouldBan && AddEntry(pNetChan->GetAddress(), pClient->GetNucleusID()) && !bSave)
					bSave = true;

				pClient->Disconnect(REP_MARK_BAD, reason);
				bDisconnect = true;
			}
		}
	}

	if (bSave)
	{
		Save();
		DevMsg(eDLL_T::SERVER, "Added '%s' to banned list\n", playerName);
	}
	else if (bDisconnect)
	{
		DevMsg(eDLL_T::SERVER, "Kicked '%s' from server\n", playerName);
	}
}

//-----------------------------------------------------------------------------
// Purpose: authors player by given nucleus id or ip address
// Input  : *playerHandle - 
//			shouldBan     - (only kicks if false)
//			*reason - 
//-----------------------------------------------------------------------------
void CBanSystem::AuthorPlayerById(const char* playerHandle, const bool shouldBan, const char* reason)
{
	Assert(VALID_CHARSTAR(playerHandle));

	try
	{
		bool bOnlyDigits = StringIsDigit(playerHandle);
		bool bDisconnect = false;
		bool bSave = false;

		if (!reason)
			reason = shouldBan ? "Banned from server" : "Kicked from server";

		for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
		{
			CClient* pClient = g_pServer->GetClient(i);
			if (!pClient)
				continue;

			CNetChan* pNetChan = pClient->GetNetChan();
			if (!pNetChan)
				continue;

			if (bOnlyDigits)
			{
				uint64_t nTargetID = static_cast<uint64_t>(std::stoll(playerHandle));
				if (nTargetID > static_cast<uint64_t>(MAX_PLAYERS)) // Is it a possible nucleusID?
				{
					uint64_t nNucleusID = pClient->GetNucleusID();
					if (nNucleusID != nTargetID)
						continue;
				}
				else // If its not try by handle.
				{
					uint64_t nClientID = static_cast<uint64_t>(pClient->GetHandle());
					if (nClientID != nTargetID)
						continue;
				}

				if (shouldBan && AddEntry(pNetChan->GetAddress(), pClient->GetNucleusID()) && !bSave)
					bSave = true;

				pClient->Disconnect(REP_MARK_BAD, reason);
				bDisconnect = true;
			}
			else
			{
				if (strcmp(playerHandle, pNetChan->GetAddress()) != NULL)
					continue;

				if (shouldBan && AddEntry(pNetChan->GetAddress(), pClient->GetNucleusID()) && !bSave)
					bSave = true;

				pClient->Disconnect(REP_MARK_BAD, reason);
				bDisconnect = true;
			}
		}

		if (bSave)
		{
			Save();
			DevMsg(eDLL_T::SERVER, "Added '%s' to banned list\n", playerHandle);
		}
		else if (bDisconnect)
		{
			DevMsg(eDLL_T::SERVER, "Kicked '%s' from server\n", playerHandle);
		}
	}
	catch (const std::exception& e)
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - %s\n", __FUNCTION__, e.what());
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
CBanSystem* g_pBanSystem = new CBanSystem();
