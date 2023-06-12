//===============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//===============================================================================//
// netmessages.cpp: implementation of the CNetMessage types.
//
///////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/net.h"
#include "common/netmessages.h"
#include "game/shared/usermessages.h"

///////////////////////////////////////////////////////////////////////////////////
// re-implementation of 'SVC_Print::Process'
///////////////////////////////////////////////////////////////////////////////////
bool SVC_Print::ProcessImpl()
{
	if (this->m_szText)
	{
		Assert(m_szText == m_szTextBuffer); // Should always point to 'm_szTextBuffer'.

		size_t len = strnlen_s(m_szText, sizeof(m_szTextBuffer));
		Assert(len < sizeof(m_szTextBuffer));

		if (len < sizeof(m_szTextBuffer))
		{
			DevMsg(eDLL_T::SERVER, m_szText[len-1] == '\n' ? "%s" : "%s\n", m_szText);
		}
	}

	return true; // Original just return true also.
}

///////////////////////////////////////////////////////////////////////////////////
// re-implementation of 'SVC_UserMessage::Process'
///////////////////////////////////////////////////////////////////////////////////
bool SVC_UserMessage::ProcessImpl()
{
	if (m_nMsgType == UserMessages_t::TextMsg)
	{
		bf_read buf = m_DataIn;
		byte type = byte(buf.ReadByte());

		if (type == HUD_PRINTCONSOLE ||
			type == HUD_PRINTCENTER)
		{
			char text[MAX_USER_MSG_DATA];
			int len;

			buf.ReadString(text, sizeof(text), false, &len);
			Assert(len < sizeof(text));

			if (len && len < sizeof(text))
			{
				DevMsg(eDLL_T::SERVER, text[len - 1] == '\n' ? "%s" : "%s\n", text);
			}
		}
	}

	return SVC_UserMessage_Process(this); // Need to return original.
}

///////////////////////////////////////////////////////////////////////////////////
// below functions are hooked as 'CmdKeyValues' isn't really used in this game, but
// still exploitable on the server. the 'OnPlayerAward' command calls the function
// 'UTIL_SendClientCommandKVToPlayer' which forwards the keyvalues to all connected clients.
///////////////////////////////////////////////////////////////////////////////////
bool Base_CmdKeyValues::ReadFromBufferImpl(Base_CmdKeyValues* thisptr, bf_read* buffer)
{
	// Abusable netmsg; only allow if cheats are enabled.
	if (!sv_cheats->GetBool())
	{
		return false;
	}

	return Base_CmdKeyValues_ReadFromBuffer(thisptr, buffer);
}
bool Base_CmdKeyValues::WriteToBufferImpl(Base_CmdKeyValues* thisptr, bf_write* buffer)
{
	// Abusable netmsg; only allow if cheats are enabled.
	if (!sv_cheats->GetBool())
	{
		return false;
	}

	return Base_CmdKeyValues_WriteToBuffer(thisptr, buffer);
}

///////////////////////////////////////////////////////////////////////////////////
// determine whether or not the message should be copied into the replay buffer,
// regardless of the 'CNetMessage::m_Group' type.
///////////////////////////////////////////////////////////////////////////////////
bool ShouldReplayMessage(const CNetMessage* msg)
{
	switch (msg->GetType())
	{
	// String commands can be abused in a way they get executed
	// on the client that is watching a replay. This happens as
	// the server copies the message into the replay buffer from
	// the client that initially submitted it. Its group type is
	// 'None', so call this to determine whether or not to set
	// the group type to 'NoReplay'. This exploit has been used
	// to connect clients to an arbitrary server during replay.
	case NetMessageType::net_StringCmd:
	// Print and user messages sometimes make their way to the
	// client that is watching a replay, while it should only
	// be broadcasted to the target client. This happens for the 
	// same reason as the 'net_StringCmd' above.
	case NetMessageType::svc_Print:
	{
		return false;
	}
	case NetMessageType::svc_UserMessage:
	{
		SVC_UserMessage* userMsg = (SVC_UserMessage*)msg;

		// Just don't replay console prints.
		if (userMsg->m_nMsgType == UserMessages_t::TextMsg)
		{
			return false;
		}

		return true;
	}
	default:
	{
		return true;
	}
	}
}

void V_NetMessages::Attach() const
{
#if !defined(DEDICATED)
	auto hk_SVCPrint_Process = &SVC_Print::ProcessImpl;
	auto hk_SVCUserMessage_Process = &SVC_UserMessage::ProcessImpl;
	auto hk_Base_CmdKeyValues_ReadFromBuffer = &Base_CmdKeyValues::ReadFromBufferImpl;
	auto hk_Base_CmdKeyValues_WriteToBuffer = &Base_CmdKeyValues::WriteToBufferImpl;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VFTable,         (LPVOID&)hk_SVCPrint_Process,       NetMessageVtbl::Process, (LPVOID*)&SVC_Print_Process);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VFTable,   (LPVOID&)hk_SVCUserMessage_Process, NetMessageVtbl::Process, (LPVOID*)&SVC_UserMessage_Process);
	CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID&)hk_Base_CmdKeyValues_ReadFromBuffer, NetMessageVtbl::ReadFromBuffer, (LPVOID*)&Base_CmdKeyValues_ReadFromBuffer);
	CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID&)hk_Base_CmdKeyValues_WriteToBuffer, NetMessageVtbl::WriteToBuffer, (LPVOID*)&Base_CmdKeyValues_WriteToBuffer);
#endif // DEDICATED
}

void V_NetMessages::Detach() const
{
#if !defined(DEDICATED)
	void* hkRestore = nullptr;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VFTable,       (LPVOID)SVC_Print_Process,       NetMessageVtbl::Process, (LPVOID*)&hkRestore);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VFTable, (LPVOID)SVC_UserMessage_Process, NetMessageVtbl::Process, (LPVOID*)&hkRestore);
	CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID)Base_CmdKeyValues_ReadFromBuffer, NetMessageVtbl::ReadFromBuffer, (LPVOID*)&hkRestore);
	CMemory::HookVirtualMethod((uintptr_t)g_pBase_CmdKeyValues_VFTable, (LPVOID)Base_CmdKeyValues_WriteToBuffer, NetMessageVtbl::WriteToBuffer, (LPVOID*)&hkRestore);
#endif // DEDICATED
}