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
	bf_read buf = m_DataIn;
	int type = buf.ReadByte();

	if (type == HUD_PRINTCONSOLE ||
		type == HUD_PRINTCENTER)
	{
		char text[MAX_USER_MSG_DATA];
		int len;

		buf.ReadString(text, sizeof(text), false, &len);
		Assert(len < sizeof(text));

		if (len >= NET_MIN_MESSAGE && len < sizeof(text))
		{
			DevMsg(eDLL_T::SERVER, text[len-1] == '\n' ? "%s" : "%s\n", text);
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