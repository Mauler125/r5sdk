//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#include "engine/sys_utils.h"
#include "engine/baseclient.h"
#include "public/include/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBanSystem::CBanSystem(void)
{
	Load();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBanSystem::operator[](std::pair<std::string, std::int64_t> pair)
{
	AddEntry(pair.first, pair.second);
}

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Load(void)
{
	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json";

	nlohmann::json jsIn;
	std::ifstream banFile(path, std::ios::in);

	int nTotalBans = 0;

	if (banFile.good() && banFile) // Check if it parsed.
	{
		banFile >> jsIn; // Into json.
		banFile.close();

		if (!jsIn.is_null()) // Check if json is valid
		{
			if (!jsIn["totalBans"].is_null())
			{
				nTotalBans = jsIn["totalBans"].get<int>();
			}
		}

		for (int i = 0; i < nTotalBans; i++)
		{
			nlohmann::json jsEntry = jsIn[std::to_string(i).c_str()]; // Get Entry for current ban.
			if (jsEntry.is_null()) // Check if entry is valid.
			{
				continue;
			}

			std::int64_t nOriginID  = jsEntry["originID"].get<std::int64_t>(); // Get originID field from entry.
			std::string svIpAddress = jsEntry["ipAddress"].get<std::string>(); // Get ipAddress field from entry.

			vsvBanList.push_back(std::make_pair(svIpAddress, nOriginID));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Save(void) const
{
	nlohmann::json jsOut;

	for (int i = 0; i < vsvBanList.size(); i++)
	{
		jsOut["totalBans"] = vsvBanList.size(); // Populate totalBans field.
		jsOut[std::to_string(i).c_str()]["ipAddress"] = vsvBanList[i].first; // Populate ipAddress field for this entry.
		jsOut[std::to_string(i).c_str()]["originID"] = vsvBanList[i].second; // Populate originID field for this entry.
	}

	fs::path path = std::filesystem::current_path() /= "platform\\banlist.json";
	std::ofstream outFile(path, std::ios::out | std::ios::trunc); // Write config file..

	outFile << jsOut.dump(4); // Dump it into config file..
	outFile.close(); // Close the file handle.
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banlist
//-----------------------------------------------------------------------------
void CBanSystem::AddEntry(std::string svIpAddress, std::int64_t nOriginID)
{
	if (!svIpAddress.empty() && nOriginID > 0) // Check if args are valid.
	{
		auto it = std::find(vsvBanList.begin(), vsvBanList.end(), std::make_pair(svIpAddress, nOriginID)); // Check if we have this entry already.
		if (it == vsvBanList.end()) // We don't have that entry?
		{
			vsvBanList.push_back(std::make_pair(svIpAddress, nOriginID)); // Add it.
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the banlist
//-----------------------------------------------------------------------------
void CBanSystem::DeleteEntry(std::string svIpAddress, std::int64_t nOriginID)
{
	for (int i = 0; i < vsvBanList.size(); i++) // Loop through vector.
	{
		if (svIpAddress.compare(vsvBanList[i].first) == NULL || nOriginID == vsvBanList[i].second) // Do any entries match our vector?
		{
			vsvBanList.erase(vsvBanList.begin() + i); // If so erase that vector element.
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds a connect refuse entry to the refuselist
//-----------------------------------------------------------------------------
void CBanSystem::AddConnectionRefuse(std::string svError, std::int64_t nOriginID)
{
	if (vsvrefuseList.empty())
	{
		vsvrefuseList.push_back(std::make_pair(svError, nOriginID));
	}
	else
	{
		for (int i = 0; i < vsvrefuseList.size(); i++) // Loop through vector.
		{
			if (vsvrefuseList[i].second != nOriginID) // Do any entries match our vector?
			{
				vsvrefuseList.push_back(std::make_pair(svError, nOriginID)); // Push it back into the vector.
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the refuselist
//-----------------------------------------------------------------------------
void CBanSystem::DeleteConnectionRefuse(std::int64_t nOriginID)
{
	for (int i = 0; i < vsvrefuseList.size(); i++) // Loop through vector.
	{
		if (vsvrefuseList[i].second == nOriginID) // Do any entries match our vector?
		{
			vsvrefuseList.erase(vsvrefuseList.begin() + i); // If so erase that vector element.
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if specified ip address or necleus id is banned
// Input  : svIpAddress - 
//			nOriginID - 
// Output : true if banned, false if not banned
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanned(std::string svIpAddress, std::int64_t nOriginID) const
{
	for (int i = 0; i < vsvBanList.size(); i++)
	{
		std::string ipAddress = vsvBanList[i].first; // Get first pair entry.
		std::int64_t originID = vsvBanList[i].second; // Get second pair entry.

		if (ipAddress.empty()) // Check if ip is empty.
		{
			continue;
		}

		if (originID <= 0) // Is originID below 0?
		{
			continue;
		}

		if (ipAddress.compare(svIpAddress) == NULL) // Do they match?
		{
			return true;
		}

		if (nOriginID == originID) // Do they match?
		{
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
		for (int i = 0; i < vsvrefuseList.size(); i++)
		{
			for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
			{
				CBaseClient* pClient = g_pClient->GetClient(c);
				CNetChan* pNetChan = pClient->GetNetChan();

				if (!pClient || !pNetChan)
				{
					continue;
				}

				if (pClient->GetOriginID() != vsvrefuseList[i].second) // See if NucleusID matches entry.
				{
					continue;
				}

				std::string svIpAddress = pNetChan->GetAddress();

				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%lld' is banned from this server!)\n", svIpAddress.c_str(), pClient->GetOriginID());
				AddEntry(svIpAddress, pClient->GetOriginID()); // Add local entry to reserve a non needed request.
				Save(); // Save banlist to file.
				NET_DisconnectClient(pClient, c, vsvrefuseList[i].first.c_str(), 0, 1);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks if refuselist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsRefuseListValid(void) const
{
	return !vsvrefuseList.empty();
}

//-----------------------------------------------------------------------------
// Purpose: checks if banlist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanListValid(void) const
{
	return !vsvBanList.empty();
}
///////////////////////////////////////////////////////////////////////////////
CBanSystem* g_pBanSystem = new CBanSystem();
