#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	ConnectClientFn originalConnectClient = nullptr;;
}

void IsClientBanned(R5Net::Client* r5net, const std::string ip, std::int64_t orid)
{
	std::string err = std::string();
	bool compBanned = r5net && r5net->GetClientIsBanned(ip, orid, err);
	if (compBanned)
	{
		while (compBanned)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
			{
				CClient* client = GameGlobals::Client->GetClientInstance(i); // Get client instance.
				if (!client) // Client instance valid?
					continue;

				if (!client->GetNetChan()) // Netchan valid?
					continue;

				std::int64_t originID = client->m_iOriginID; // Get originID.
				if (originID != orid) // See if they match.
					continue;

				GameGlobals::BanSystem->AddConnectionRefuse(err, client->m_iUserID + 1); // Add to the vector.
				compBanned = false;
				break;
			}
		}
	}
}

void* Hooks::ConnectClient(void* thisptr, void* packet)
{
	if (!GameGlobals::BanSystem)
		return originalConnectClient(thisptr, packet);

	std::string finalIPAddress = "null";
	MemoryAddress ipAddressField = MemoryAddress(((std::uintptr_t)packet + 0x10));
	if (ipAddressField && ipAddressField.GetValue<int>() != 0x0)
	{
		std::stringstream ss;
		ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
			<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
			<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
			<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

		finalIPAddress = ss.str();
	}

	const char* name = *(const char**)((std::uintptr_t)packet + 0x30);
	std::int64_t originID = *(std::int64_t*)((std::uintptr_t)packet + 0x28);

	g_GameConsole->AddLog("[CServer::ConnectClient] %s is trying to connect. OriginID: %lld", name, originID);

	if (GameGlobals::BanSystem->IsBanListValid()) // Is the banlist vector valid?
	{
		if (GameGlobals::BanSystem->IsBanned(finalIPAddress, originID)) // Is the client trying to connect banned?
		{
			addr_CServer_RejectConnection(thisptr, *(unsigned int*)((std::uintptr_t)thisptr + 0xC), packet, "You have been banned from this Server."); // RejectConnection for the client.
			g_GameConsole->AddLog("[CServer::ConnectClient] %s is banned. OriginID: %lld", name, originID);
			return nullptr;
		}
	}

	if (g_CheckCompBanDB)
	{
		if (g_ServerBrowser)
		{
			R5Net::Client* r5net = g_ServerBrowser->GetR5Net();
			if (r5net)
			{
				std::thread t1(IsClientBanned, r5net, finalIPAddress, originID);
				t1.detach();
			}
		}
	}

	return originalConnectClient(thisptr, packet);
}