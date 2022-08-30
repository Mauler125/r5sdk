//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#include "engine/client/client.h"
#include "networksystem/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Load(void)
{
	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json"; // !TODO: Use FS "PLATFORM"
	nlohmann::json jsIn;

	ifstream banFile(path, std::ios::in);
	int nTotalBans = 0;

	if (IsBanListValid())
	{
		m_vBanList.clear();
	}

	if (banFile.good() && banFile)
	{
		banFile >> jsIn; // Into json.
		banFile.close();

		if (!jsIn.is_null())
		{
			if (!jsIn["totalBans"].is_null())
			{
				nTotalBans = jsIn["totalBans"].get<int>();
			}
		}

		for (int i = 0; i < nTotalBans; i++)
		{
			nlohmann::json jsEntry = jsIn[std::to_string(i)];
			if (jsEntry.is_null())
			{
				continue;
			}

			uint64_t nNucleusID = jsEntry["nucleusId"].get<uint64_t>();
			string svIpAddress = jsEntry["ipAddress"].get<string>();

			m_vBanList.push_back(std::make_pair(svIpAddress, nNucleusID));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Save(void) const
{
	nlohmann::json jsOut;

	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		jsOut[std::to_string(i)]["ipAddress"] = m_vBanList[i].first;
		jsOut[std::to_string(i)]["nucleusId"] = m_vBanList[i].second;
	}
	jsOut["totalBans"] = m_vBanList.size();

	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json"; // !TODO: Use FS "PLATFORM".
	ofstream outFile(path, std::ios::out | std::ios::trunc); // Write config file..

	outFile << jsOut.dump(4);
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banlist
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
// Purpose: deletes an entry in the banlist
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
// Purpose: adds a connect refuse entry to the refuselist
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
// Purpose: deletes an entry in the refuselist
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
	if (IsRefuseListValid())
	{
		bool bSave = false;
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

				if (pClient->GetNucleusID() != m_vRefuseList[i].second)
					continue;

				string svIpAddress = pNetChan->GetAddress();

				Warning(eDLL_T::SERVER, "Removing client '%s' from slot '%hu' ('%llu' is banned from this server!)\n", svIpAddress.c_str(), pClient->GetHandle(), pClient->GetNucleusID());
				if (AddEntry(svIpAddress, pClient->GetNucleusID()) && !bSave)
					bSave = true;

				NET_DisconnectClient(pClient, c, m_vRefuseList[i].first.c_str(), 0, true);
			}
		}

		if (bSave)
			Save(); // Save banlist to file.
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if specified ip address or necleus id is banned
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
// Purpose: checks if refuselist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsRefuseListValid(void) const
{
	return !m_vRefuseList.empty();
}

//-----------------------------------------------------------------------------
// Purpose: checks if banlist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanListValid(void) const
{
	return !m_vBanList.empty();
}
///////////////////////////////////////////////////////////////////////////////
CBanSystem* g_pBanSystem = new CBanSystem();
