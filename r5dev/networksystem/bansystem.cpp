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
// Purpose: 
//-----------------------------------------------------------------------------
CBanSystem::CBanSystem(void)
{
	Load();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pair<const string&, const uint64_t> - 
//-----------------------------------------------------------------------------
void CBanSystem::operator[](std::pair<const string&, const uint64_t> pair)
{
	AddEntry(pair.first, pair.second);
}

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Load(void)
{
	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json"; // !TODO: Use FS "PLATFORM"

	nlohmann::json jsIn;
	ifstream banFile(path, std::ios::in);

	int nTotalBans = 0;

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

			uint64_t nOriginID = jsEntry["originID"].get<uint64_t>();
			string svIpAddress = jsEntry["ipAddress"].get<string>();

			m_vBanList.push_back(std::make_pair(svIpAddress, nOriginID));
		}
	}
	else
	{
		// File no longer accessible, assume they want all bans dropped.
		m_vBanList.clear();
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
		jsOut["totalBans"] = m_vBanList.size();
		jsOut[std::to_string(i)]["ipAddress"] = m_vBanList[i].first;
		jsOut[std::to_string(i)]["originID"] = m_vBanList[i].second;
	}

	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json"; // !TODO: Use FS "PLATFORM".
	ofstream outFile(path, std::ios::out | std::ios::trunc); // Write config file..

	outFile << jsOut.dump(4);
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banlist
// Input  : &svIpAddress - 
//			nOriginID - 
//-----------------------------------------------------------------------------
bool CBanSystem::AddEntry(const string& svIpAddress, const uint64_t nOriginID)
{
	if (!svIpAddress.empty())
	{
		auto it = std::find(m_vBanList.begin(), m_vBanList.end(), std::make_pair(svIpAddress, nOriginID));
		if (it == m_vBanList.end())
		{
			m_vBanList.push_back(std::make_pair(svIpAddress, nOriginID));
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the banlist
// Input  : &svIpAddress - 
//			nOriginID - 
//-----------------------------------------------------------------------------
bool CBanSystem::DeleteEntry(const string& svIpAddress, const uint64_t nOriginID)
{
	bool result = false;
	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		if (svIpAddress.compare(m_vBanList[i].first) == NULL || nOriginID == m_vBanList[i].second)
		{
			m_vBanList.erase(m_vBanList.begin() + i);
			result = true;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: adds a connect refuse entry to the refuselist
// Input  : &svError - 
//			nOriginID - 
//-----------------------------------------------------------------------------
void CBanSystem::AddConnectionRefuse(const string& svError, const uint64_t nOriginID)
{
	if (m_vRefuseList.empty())
	{
		m_vRefuseList.push_back(std::make_pair(svError, nOriginID));
	}
	else
	{
		for (size_t i = 0; i < m_vRefuseList.size(); i++)
		{
			if (m_vRefuseList[i].second != nOriginID)
			{
				m_vRefuseList.push_back(std::make_pair(svError, nOriginID));
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the refuselist
// Input  : nOriginID - 
//-----------------------------------------------------------------------------
void CBanSystem::DeleteConnectionRefuse(const uint64_t nOriginID)
{
	for (size_t i = 0; i < m_vRefuseList.size(); i++)
	{
		if (m_vRefuseList[i].second == nOriginID)
		{
			m_vRefuseList.erase(m_vRefuseList.begin() + i);
		}
	}
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

				if (pClient->GetOriginID() != m_vRefuseList[i].second)
					continue;

				string svIpAddress = pNetChan->GetAddress();

				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' is banned from this server!)\n", svIpAddress.c_str(), pClient->GetOriginID());
				NET_DisconnectClient(pClient, c, m_vRefuseList[i].first.c_str(), 0, true);

				if (AddEntry(svIpAddress, pClient->GetOriginID() && !bSave))
					bSave = true;
			}
		}

		if (bSave)
			Save(); // Save banlist to file.
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if specified ip address or necleus id is banned
// Input  : &svIpAddress - 
//			nOriginID - 
// Output : true if banned, false if not banned
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanned(const string& svIpAddress, const uint64_t nOriginID) const
{
	for (size_t i = 0; i < m_vBanList.size(); i++)
	{
		string ipAddress = m_vBanList[i].first;
		uint64_t originID = m_vBanList[i].second;

		if (ipAddress.empty() ||
			!originID) // Cannot be null.
		{
			continue;
		}

		if (ipAddress.compare(svIpAddress) == NULL ||
			nOriginID == originID)
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
