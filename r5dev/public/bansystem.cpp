//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "client/client.h"
#include "engine/net_chan.h"
#include "engine/sys_utils.h"
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
void CBanSystem::Save(void) const
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
		for (int i = 0; i < vsvrefuseList.size(); i++) // Loop through vector.
		{
			for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
			{
				CClient* client = g_pClient->GetClientInstance(c); // Get client instance.
				if (!client)
				{
					continue;
				}

				if (!client->GetNetChan()) // Netchan valid?
				{
					continue;
				}

				if (g_pClient->m_iOriginID != vsvrefuseList[i].second) // See if nucleus id matches entry.
				{
					continue;
				}

				std::string finalIpAddress = std::string();
				ADDRESS ipAddressField = ADDRESS(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
				if (ipAddressField && ipAddressField.GetValue<int>() != 0x0)
				{
					std::stringstream ss;
					ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

					finalIpAddress = ss.str();
				}

				DevMsg(eDLL_T::SERVER, "\n");
				DevMsg(eDLL_T::SERVER, "______________________________________________________________\n");
				DevMsg(eDLL_T::SERVER, "] PYLON_NOTICE -----------------------------------------------\n");
				DevMsg(eDLL_T::SERVER, "] OriginID : | '%lld' IS GETTING DISCONNECTED.\n", g_pClient->m_iOriginID);
				if (finalIpAddress.empty())
					DevMsg(eDLL_T::SERVER, "] IP-ADDR  : | CLIENT MODIFIED PACKET.\n");
				else
					DevMsg(eDLL_T::SERVER, "] IP-ADDR  : | '%s'\n", finalIpAddress.c_str());
				DevMsg(eDLL_T::SERVER, "--------------------------------------------------------------\n");
				DevMsg(eDLL_T::SERVER, "\n");

				AddEntry(finalIpAddress, g_pClient->m_iOriginID); // Add local entry to reserve a non needed request.
				Save(); // Save list.
				NET_DisconnectClient(g_pClient, c, vsvrefuseList[i].first.c_str(), 0, 1); // Disconnect client.
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
CBanSystem* g_pBanSystem = new CBanSystem();;
