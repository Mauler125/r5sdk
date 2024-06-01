//=====================================================================================//
//
// Purpose: Implementation of the CBanSystem class.
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "tier1/strtools.h"
#include "engine/net.h"
#include "engine/server/server.h"
#include "engine/client/client.h"
#include "filesystem/filesystem.h"
#include "networksystem/bansystem.h"

//-----------------------------------------------------------------------------
// Purpose: loads and parses the banned list
//-----------------------------------------------------------------------------
void CBanSystem::LoadList(void)
{
	if (IsBanListValid())
		m_BannedList.Purge();

	FileHandle_t pFile = FileSystem()->Open("banlist.json", "rt", "PLATFORM");
	if (!pFile)
		return;

	const ssize_t nLen = FileSystem()->Size(pFile);
	std::unique_ptr<char[]> pBuf(new char[nLen + 1]);

	const ssize_t nRead = FileSystem()->Read(pBuf.get(), nLen, pFile);
	FileSystem()->Close(pFile);

	pBuf[nRead] = '\0'; // Null terminate the string buffer containing our banned list.

	rapidjson::Document document;
	if (document.Parse(pBuf.get()).HasParseError())
	{
		Warning(eDLL_T::SERVER, "%s: JSON parse error at position %zu: %s\n",
			__FUNCTION__, document.GetErrorOffset(), rapidjson::GetParseError_En(document.GetParseError()));
		return;
	}

	if (!document.IsObject())
	{
		Warning(eDLL_T::SERVER, "%s: JSON root was not an object\n", __FUNCTION__);
		return;
	}

	uint64_t nTotalBans = 0;
	if (document.HasMember("totalBans") && document["totalBans"].IsUint64())
	{
		nTotalBans = document["totalBans"].GetUint64();
	}

	for (uint64_t i = 0; i < nTotalBans; i++)
	{
		char idx[64]; _ui64toa(i, idx, 10);

		if (document.HasMember(idx) && document[idx].IsObject())
		{
			const rapidjson::Value& entry = document[idx];
			if (entry.HasMember("ipAddress") && entry["ipAddress"].IsString() &&
				entry.HasMember("nucleusId") && entry["nucleusId"].IsUint64())
			{
				Banned_t banned;
				banned.m_Address = entry["ipAddress"].GetString();
				banned.m_NucleusID = entry["nucleusId"].GetUint64();

				m_BannedList.AddToTail(banned);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves the banned list
//-----------------------------------------------------------------------------
void CBanSystem::SaveList(void) const
{
	FileHandle_t pFile = FileSystem()->Open("banlist.json", "wt", "PLATFORM");
	if (!pFile)
	{
		Error(eDLL_T::SERVER, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, "banlist.json");
		return;
	}

	rapidjson::Document document;
	document.SetObject();

	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	FOR_EACH_VEC(m_BannedList, i)
	{
		const Banned_t& banned = m_BannedList[i];
		char idx[64]; _ui64toa(i, idx, 10);

		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("ipAddress", rapidjson::Value(banned.m_Address.String(), allocator), allocator);
		obj.AddMember("nucleusId", banned.m_NucleusID, allocator);

		document.AddMember(rapidjson::Value(idx, allocator), obj, allocator);
	}

	document.AddMember("totalBans", m_BannedList.Count(), allocator);

	rapidjson::StringBuffer buffer;
	JSON_DocumentToBufferDeserialize(document, buffer);

	FileSystem()->Write(buffer.GetString(), buffer.GetSize(), pFile);
	FileSystem()->Close(pFile);
}

//-----------------------------------------------------------------------------
// Purpose: adds a banned player entry to the banned list
// Input  : *ipAddress - 
//			nucleusId - 
//-----------------------------------------------------------------------------
bool CBanSystem::AddEntry(const char* ipAddress, const NucleusID_t nucleusId)
{
	Assert(VALID_CHARSTAR(ipAddress));
	const Banned_t banned(ipAddress, nucleusId);

	if (IsBanListValid())
	{
		if (m_BannedList.Find(banned) == m_BannedList.InvalidIndex())
		{
			m_BannedList.AddToTail(banned);
			return true;
		}
	}
	else
	{
		m_BannedList.AddToTail(banned);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entry in the banned list
// Input  : *ipAddress - 
//			nucleusId - 
//-----------------------------------------------------------------------------
bool CBanSystem::DeleteEntry(const char* ipAddress, const NucleusID_t nucleusId)
{
	Assert(VALID_CHARSTAR(ipAddress));

	if (IsBanListValid())
	{
		FOR_EACH_VEC(m_BannedList, i)
		{
			const Banned_t& banned = m_BannedList[i];

			if (banned.m_NucleusID == nucleusId ||
				banned.m_Address.IsEqual_CaseInsensitive(ipAddress))
			{
				m_BannedList.Remove(i);
				return true;
			}
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
bool CBanSystem::IsBanned(const char* ipAddress, const NucleusID_t nucleusId) const
{
	FOR_EACH_VEC(m_BannedList, i)
	{
		const Banned_t& banned = m_BannedList[i];

		if (banned.m_NucleusID == NULL ||
			banned.m_Address.IsEmpty())
		{
			// Cannot be NULL.
			continue;
		}

		if (banned.m_NucleusID == nucleusId ||
			banned.m_Address.IsEqual_CaseInsensitive(ipAddress))
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
	return !m_BannedList.IsEmpty();
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
	bool bSave = false;
	if (V_IsAllDigit(criteria)) // Check if we have an ip address or nucleus id.
	{
		char* pEnd = nullptr;
		const uint64_t nTargetID = strtoull(criteria, &pEnd, 10);

		if (DeleteEntry("-<[InVaLiD]>-", nTargetID)) // Delete ban entry.
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
		SaveList(); // Save modified vector to file.
		Msg(eDLL_T::SERVER, "Removed '%s' from banned list\n", criteria);
	}
}

//-----------------------------------------------------------------------------
// Purpose: authors player by given name
// Input  : *playerName - 
//			shouldBan   - (only kicks if false)
//			*reason     - 
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

		const CNetChan* pNetChan = pClient->GetNetChan();
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
		SaveList();
		Msg(eDLL_T::SERVER, "Added '%s' to banned list\n", playerName);
	}
	else if (bDisconnect)
	{
		Msg(eDLL_T::SERVER, "Kicked '%s' from server\n", playerName);
	}
}

//-----------------------------------------------------------------------------
// Purpose: authors player by given nucleus id or ip address
// Input  : *playerHandle - 
//			shouldBan     - (only kicks if false)
//			*reason       - 
//-----------------------------------------------------------------------------
void CBanSystem::AuthorPlayerById(const char* playerHandle, const bool shouldBan, const char* reason)
{
	Assert(VALID_CHARSTAR(playerHandle));

	bool bOnlyDigits = V_IsAllDigit(playerHandle);
	bool bDisconnect = false;
	bool bSave = false;

	if (!reason)
		reason = shouldBan ? "Banned from server" : "Kicked from server";

	for (int i = 0; i < g_ServerGlobalVariables->m_nMaxClients; i++)
	{
		CClient* pClient = g_pServer->GetClient(i);
		if (!pClient)
			continue;

		const CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		if (bOnlyDigits)
		{
			char* pEnd = nullptr;
			const uint64_t nTargetID = strtoull(playerHandle, &pEnd, 10);

			if (nTargetID >= MAX_PLAYERS) // Is it a possible nucleusID?
			{
				const NucleusID_t nNucleusID = pClient->GetNucleusID();

				if (nNucleusID != nTargetID)
					continue;
			}
			else // If its not try by handle.
			{
				const edict_t nClientID = pClient->GetHandle();

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
		SaveList();
		Msg(eDLL_T::SERVER, "Added '%s' to banned list\n", playerHandle);
	}
	else if (bDisconnect)
	{
		Msg(eDLL_T::SERVER, "Kicked '%s' from server\n", playerHandle);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Console command handlers
///////////////////////////////////////////////////////////////////////////////

static void _Author_Client_f(const CCommand& args, EKickType type)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	const char* szReason = args.ArgC() > 2 ? args.Arg(2) : nullptr;

	switch (type)
	{
	case KICK_NAME:
	{
		g_BanSystem.KickPlayerByName(args.Arg(1), szReason);
		break;
	}
	case KICK_ID:
	{
		g_BanSystem.KickPlayerById(args.Arg(1), szReason);
		break;
	}
	case BAN_NAME:
	{
		g_BanSystem.BanPlayerByName(args.Arg(1), szReason);
		break;
	}
	case BAN_ID:
	{
		g_BanSystem.BanPlayerById(args.Arg(1), szReason);
		break;
	}
	default:
	{
		// Code bug.
		Assert(0);
	}
	}
}
static void Host_Kick_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::KICK_NAME);
}
static void Host_KickID_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::KICK_ID);
}
static void Host_Ban_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::BAN_NAME);
}
static void Host_BanID_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::BAN_ID);
}
static void Host_Unban_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	g_BanSystem.UnbanPlayer(args.Arg(1));
}
static void Host_ReloadBanList_f()
{
	g_BanSystem.LoadList(); // Reload banned list.
}

static ConCommand kick("kick", Host_Kick_f, "Kick a client from the server by user name", FCVAR_RELEASE, nullptr, "kick \"<userId>\"");
static ConCommand kickid("kickid", Host_KickID_f, "Kick a client from the server by handle, nucleus id or ip address", FCVAR_RELEASE, nullptr, "kickid \"<handle>\"/\"<nucleusId>/<ipAddress>\"");
static ConCommand ban("ban", Host_Ban_f, "Bans a client from the server by user name", FCVAR_RELEASE, nullptr, "ban <userId>");
static ConCommand banid("banid", Host_BanID_f, "Bans a client from the server by handle, nucleus id or ip address", FCVAR_RELEASE, nullptr, "banid \"<handle>\"/\"<nucleusId>/<ipAddress>\"");
static ConCommand unban("unban", Host_Unban_f, "Unbans a client from the server by nucleus id or ip address", FCVAR_RELEASE, nullptr, "unban \"<nucleusId>\"/\"<ipAddress>\"");
static ConCommand reload_banlist("banlist_reload", Host_ReloadBanList_f, "Reloads the banned list", FCVAR_RELEASE);

///////////////////////////////////////////////////////////////////////////////
CBanSystem g_BanSystem;
