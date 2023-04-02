//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
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

	uint32_t nLen = FileSystem()->Size(pFile);
	char* pBuf = MemAllocSingleton()->Alloc<char>(nLen + 1);

	int nRead = FileSystem()->Read(pBuf, nLen, pFile);
	FileSystem()->Close(pFile);

	pBuf[nRead] = '\0'; // Null terminate the string buffer containing our banned list.

	try
	{
		nlohmann::json jsIn = nlohmann::json::parse(pBuf);

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
				string  svIpAddress = jsEntry["ipAddress"].get<string>();
				uint64_t nNucleusID = jsEntry["nucleusId"].get<uint64_t>();

				m_vBanList.push_back(std::make_pair(svIpAddress, nNucleusID));
			}
		}
	}
	catch (const std::exception& ex)
	{
		Warning(eDLL_T::SERVER, "%s: Exception while parsing banned list:\n%s\n", __FUNCTION__, ex.what());
	}

	MemAllocSingleton()->Free(pBuf);
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

		FileSystem()->Write(svJsOut.data(), int(svJsOut.size()), pFile);
	}
	catch (const std::exception& ex)
	{
		Warning(eDLL_T::SERVER, "%s: Exception while parsing banned list:\n%s\n", __FUNCTION__, ex.what());
	}

	FileSystem()->Close(pFile);
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banned list
// Input  : &svIpAddress - 
//			nNucleusID - 
//-----------------------------------------------------------------------------
bool CBanSystem::AddEntry(const string& svIpAddress, const uint64_t nNucleusID)
{
	Assert(!svIpAddress.empty());

	if (IsBanListValid())
	{
		auto it = std::find(m_vBanList.begin(), m_vBanList.end(), std::make_pair(svIpAddress, nNucleusID));
		if (it == m_vBanList.end())
		{
			m_vBanList.push_back(std::make_pair(svIpAddress, nNucleusID));
			return true;
		}
	}
	else
	{
		m_vBanList.push_back(std::make_pair(svIpAddress, nNucleusID));
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the banned list
// Input  : &svIpAddress - 
//			nNucleusID - 
//-----------------------------------------------------------------------------
bool CBanSystem::DeleteEntry(const string& svIpAddress, const uint64_t nNucleusID)
{
	Assert(!svIpAddress.empty());

	if (IsBanListValid())
	{
		auto it = std::find_if(m_vBanList.begin(), m_vBanList.end(),
			[&](const pair<const string, const uint64_t>& element)
			{ return (svIpAddress.compare(element.first) == NULL || element.second == nNucleusID); });

		if (it != m_vBanList.end())
		{
			DeleteConnectionRefuse(it->second);
			m_vBanList.erase(it);

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: adds a connect refuse entry to the refused list
// Input  : &svError - 
//			nNucleusID - 
//-----------------------------------------------------------------------------
bool CBanSystem::AddConnectionRefuse(const string& svError, const uint64_t nNucleusID)
{
	if (IsRefuseListValid())
	{
		auto it = std::find_if(m_vRefuseList.begin(), m_vRefuseList.end(),
			[&](const pair<const string, const uint64_t>& element) { return element.second == nNucleusID; });

		if (it == m_vRefuseList.end())
		{
			m_vRefuseList.push_back(std::make_pair(svError, nNucleusID));
			return true;
		}
	}
	else
	{
		m_vRefuseList.push_back(std::make_pair(svError, nNucleusID));
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the refused list
// Input  : nNucleusID - 
//-----------------------------------------------------------------------------
bool CBanSystem::DeleteConnectionRefuse(const uint64_t nNucleusID)
{
	if (IsRefuseListValid())
	{
		auto it = std::find_if(m_vRefuseList.begin(), m_vRefuseList.end(),
			[&](const pair<const string, const uint64_t>& element) { return element.second == nNucleusID; });

		if (it != m_vRefuseList.end())
		{
			m_vRefuseList.erase(it);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check refuse list and kill netchan connection.
//-----------------------------------------------------------------------------
void CBanSystem::BanListCheck(void)
{
	if (!IsRefuseListValid())
		return;

	for (size_t i = 0; i < m_vRefuseList.size(); i++)
	{
		for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
		{
			CClient* pClient = g_pClient->GetClient(c);
			if (!pClient)
				continue;

			CNetChan* pNetChan = pClient->GetNetChan();
			if (!pNetChan)
				continue;

			if (!pClient->IsConnected())
				continue;

			if (pClient->GetNucleusID() != m_vRefuseList[i].second)
				continue;

			string svIpAddress = pNetChan->GetAddress();

			Warning(eDLL_T::SERVER, "Removing client '%s' from slot '%i' ('%llu' is banned from this server!)\n", svIpAddress.c_str(), c, pClient->GetNucleusID());
			pClient->Disconnect(Reputation_t::REP_MARK_BAD, m_vRefuseList[i].first.c_str());
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if specified ip address or nucleus id is banned
// Input  : &svIpAddress - 
//			nNucleusID - 
// Output : true if banned, false if not banned
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanned(const string& svIpAddress, const uint64_t nNucleusID) const
{
	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		string ipAddress = m_vBanList[i].first;
		uint64_t nucleusID = m_vBanList[i].second;

		if (ipAddress.empty() ||
			!nucleusID) // Cannot be null.
		{
			continue;
		}

		if (ipAddress.compare(svIpAddress) == NULL ||
			nNucleusID == nucleusID)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if refused list is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsRefuseListValid(void) const
{
	return !m_vRefuseList.empty();
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
// Input  : &svPlayerName - 
//-----------------------------------------------------------------------------
void CBanSystem::KickPlayerByName(const string& svPlayerName)
{
	if (svPlayerName.empty())
		return;

	AuthorPlayerByName(svPlayerName, false);
}

//-----------------------------------------------------------------------------
// Purpose: kicks a player by given handle or id
// Input  : &svHandle - 
//-----------------------------------------------------------------------------
void CBanSystem::KickPlayerById(const string& svHandle)
{
	if (svHandle.empty())
		return;

	AuthorPlayerById(svHandle, false);
}

//-----------------------------------------------------------------------------
// Purpose: bans a player by given name
// Input  : &svPlayerName - 
//-----------------------------------------------------------------------------
void CBanSystem::BanPlayerByName(const string& svPlayerName)
{
	if (svPlayerName.empty())
		return;

	AuthorPlayerByName(svPlayerName, true);
}

//-----------------------------------------------------------------------------
// Purpose: bans a player by given handle or id
// Input  : &svHandle - 
//-----------------------------------------------------------------------------
void CBanSystem::BanPlayerById(const string& svHandle)
{
	if (svHandle.empty())
		return;

	AuthorPlayerById(svHandle, true);
}

//-----------------------------------------------------------------------------
// Purpose: unbans a player by given nucleus id or ip address
// Input  : &svCriteria - 
//-----------------------------------------------------------------------------
void CBanSystem::UnbanPlayer(const string& svCriteria)
{
	try
	{
		bool bSave = false;
		if (StringIsDigit(svCriteria)) // Check if we have an ip address or nucleus id.
		{
			if (DeleteEntry("<<invalid>>", std::stoll(svCriteria))) // Delete ban entry.
			{
				bSave = true;
			}
		}
		else
		{
			if (DeleteEntry(svCriteria, 0)) // Delete ban entry.
			{
				bSave = true;
			}
		}

		if (bSave)
		{
			Save(); // Save modified vector to file.
			DevMsg(eDLL_T::SERVER, "Removed '%s' from banned list\n", svCriteria.c_str());
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
// Input  : &svPlayerName - 
//			bBan          - (only kicks if false)
//-----------------------------------------------------------------------------
void CBanSystem::AuthorPlayerByName(const string& svPlayerName, const bool bBan)
{
	Assert(!svPlayerName.empty());
	bool bDisconnect = false;
	bool bSave = false;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		if (strlen(pNetChan->GetName()) > 0)
		{
			if (svPlayerName.compare(pNetChan->GetName()) == NULL) // Our wanted name?
			{
				if (bBan && AddEntry(pNetChan->GetAddress(), pClient->GetNucleusID()) && !bSave)
					bSave = true;

				pClient->Disconnect(REP_MARK_BAD, bBan ? "Banned from server" : "Kicked from server");
				bDisconnect = true;
			}
		}
	}

	if (bSave)
	{
		Save();
		DevMsg(eDLL_T::SERVER, "Added '%s' to banned list\n", svPlayerName.c_str());
	}
	else if (bDisconnect)
	{
		DevMsg(eDLL_T::SERVER, "Kicked '%s' from server\n", svPlayerName.c_str());
	}
}

//-----------------------------------------------------------------------------
// Purpose: authors player by given nucleus id or ip address
// Input  : &svHandle - 
//			bBan      - (only kicks if false)
//-----------------------------------------------------------------------------
void CBanSystem::AuthorPlayerById(const string& svHandle, const bool bBan)
{
	Assert(!svHandle.empty());

	try
	{
		bool bOnlyDigits = StringIsDigit(svHandle);
		bool bDisconnect = false;
		bool bSave = false;

		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			CClient* pClient = g_pClient->GetClient(i);
			if (!pClient)
				continue;

			CNetChan* pNetChan = pClient->GetNetChan();
			if (!pNetChan)
				continue;

			if (bOnlyDigits)
			{
				uint64_t nTargetID = static_cast<uint64_t>(std::stoll(svHandle));
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

				if (bBan && AddEntry(pNetChan->GetAddress(), pClient->GetNucleusID()) && !bSave)
					bSave = true;

				pClient->Disconnect(REP_MARK_BAD, bBan ? "Banned from server" : "Kicked from server");
				bDisconnect = true;
			}
			else
			{
				if (svHandle.compare(pNetChan->GetAddress()) != NULL)
					continue;

				if (bBan && AddEntry(pNetChan->GetAddress(), pClient->GetNucleusID()) && !bSave)
					bSave = true;

				pClient->Disconnect(REP_MARK_BAD, bBan ? "Banned from server" : "Kicked from server");
				bDisconnect = true;
			}
		}

		if (bSave)
		{
			Save();
			DevMsg(eDLL_T::SERVER, "Added '%s' to banned list\n", svHandle.c_str());
		}
		else if (bDisconnect)
		{
			DevMsg(eDLL_T::SERVER, "Kicked '%s' from server\n", svHandle.c_str());
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
