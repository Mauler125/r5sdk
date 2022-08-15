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

bool SVC_Print::Process()
{
	if (this->m_szText)
	{
		DevMsg(eDLL_T::SERVER, this->m_szText);
	}

	return true; // Original just return true also.
}

bool SVC_UserMessage::Process()
{
	bf_read buf = m_DataIn;
	int type = buf.ReadByte();

	if (type == HUD_PRINTCONSOLE ||
		type == HUD_PRINTCENTER)
	{
		char text[MAX_USER_MSG_DATA];
		buf.ReadString(text, sizeof(text));
		if (strnlen_s(text, sizeof(text)) >= NET_MIN_MESSAGE)
		{
			DevMsg(eDLL_T::SERVER, text);
		}
	}

	return SVC_UserMessage_Process(this); // Need to return original.
}

void CNetMessages_Attach()
{
	auto SVCPrint = &SVC_Print::Process;
	auto SVCUserMessage = &SVC_UserMessage::Process;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable,       (LPVOID&)SVCPrint,       3, (LPVOID*)&SVC_Print_Process);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VTable, (LPVOID&)SVCUserMessage, 3, (LPVOID*)&SVC_UserMessage_Process);
}

void CNetMessages_Detach()
{
	void* hkRestore = nullptr;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable,       (LPVOID)SVC_Print_Process,       3, (LPVOID*)&hkRestore);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VTable, (LPVOID)SVC_UserMessage_Process, 3, (LPVOID*)&hkRestore);
}