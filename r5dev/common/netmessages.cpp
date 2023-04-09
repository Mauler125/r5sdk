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
#include "engine/net.h"
#include "common/netmessages.h"

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

void V_NetMessages::Attach() const
{
#if !defined(DEDICATED)
	auto SVCPrint = &SVC_Print::ProcessImpl;
	auto SVCUserMessage = &SVC_UserMessage::ProcessImpl;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VFTable,       (LPVOID&)SVCPrint,       3, (LPVOID*)&SVC_Print_Process);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VFTable, (LPVOID&)SVCUserMessage, 3, (LPVOID*)&SVC_UserMessage_Process);
#endif // DEDICATED
}

void V_NetMessages::Detach() const
{
#if !defined(DEDICATED)
	void* hkRestore = nullptr;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VFTable,       (LPVOID)SVC_Print_Process,       3, (LPVOID*)&hkRestore);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VFTable, (LPVOID)SVC_UserMessage_Process, 3, (LPVOID*)&hkRestore);
#endif // DEDICATED
}