#include "core/stdafx.h"
#include "public/include/bansystem.h"

CBanSystem::CBanSystem()
{
	Load();
}

void CBanSystem::operator[](std::pair<std::string, std::int64_t> pair)
{
	AddEntry(pair.first, pair.second);
}

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Load()
{
	std::filesystem::path path = std::filesystem::current_path() /= "banlist.config"; // Get current path + banlist.config

	nlohmann::json jsIn;
	std::ifstream banFile(path, std::ios::in); // Parse ban list.

	int nTotalBans = 0;

	if (banFile.good() && banFile) // Check if it parsed.
	{
		banFile >> jsIn; // Into json.
		banFile.close(); // Close file.

		if (!jsIn.is_null()) // Check if json is valid
		{
			if (!jsIn["totalBans"].is_null()) // Is the totalBans field populated?
			{
				nTotalBans = jsIn["totalBans"].get<int>(); // Get the totalBans field.
			}
		}

		for (int i = 0; i < nTotalBans; i++) // Loop through total bans.
		{
			nlohmann::json jsEntry = jsIn[std::to_string(i).c_str()]; // Get Entry for current ban.
			if (jsEntry.is_null()) // Check if entry is valid.
			{
				continue;
			}


			std::int64_t nOriginID  = jsEntry["originID"].get<std::int64_t>(); // Get originID field from entry.
			std::string svIpAddress = jsEntry["ipAddress"].get<std::string>(); // Get ipAddress field from entry.

			vsvBanList.push_back(std::make_pair(svIpAddress, nOriginID)); // Push back into vector.
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves the banlist
//-----------------------------------------------------------------------------
void CBanSystem::Save()
{
	nlohmann::json jsOut;

	for (int i = 0; i < vsvBanList.size(); i++)
	{
		jsOut["totalBans"] = vsvBanList.size(); // Populate totalBans field.
		jsOut[std::to_string(i).c_str()]["ipAddress"] = vsvBanList[i].first; // Populate ipAddress field for this entry.
		jsOut[std::to_string(i).c_str()]["originID"] = vsvBanList[i].second; // Populate originID field for this entry.
	}

	std::filesystem::path path = std::filesystem::current_path() /= "banlist.config"; // Get current path + banlist.config
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
		vsvBanList.push_back(std::make_pair(svIpAddress, nOriginID)); // Push it back into the vector.
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
void CBanSystem::AddConnectionRefuse(std::string svError, int nUserID)
{
	if (vsvrefuseList.empty())
	{
		vsvrefuseList.push_back(std::make_pair(svError, nUserID));
	}
	else
	{
		for (int i = 0; i < vsvrefuseList.size(); i++) // Loop through vector.
		{
			if (vsvrefuseList[i].second != nUserID) // Do any entries match our vector?
			{
				vsvrefuseList.push_back(std::make_pair(svError, nUserID)); // Push it back into the vector.
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the refuselist
//-----------------------------------------------------------------------------
void CBanSystem::DeleteConnectionRefuse(int nUserID)
{
	for (int i = 0; i < vsvrefuseList.size(); i++) // Loop through vector.
	{
		if (vsvrefuseList[i].second == nUserID) // Do any entries match our vector?
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
bool CBanSystem::IsBanned(std::string svIpAddress, std::int64_t nOriginID)
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
// Purpose: checks if refuselist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsRefuseListValid()
{
	return !vsvrefuseList.empty();
}

//-----------------------------------------------------------------------------
// Purpose: checks if banlist is valid
//-----------------------------------------------------------------------------
bool CBanSystem::IsBanListValid()
{
	return !vsvBanList.empty();
}
///////////////////////////////////////////////////////////////////////////////
CBanSystem* g_pBanSystem = new CBanSystem();;
