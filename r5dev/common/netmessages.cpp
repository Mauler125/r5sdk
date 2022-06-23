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
	UserMessages type = (UserMessages)buf.ReadByte();

	if (type == UserMessages::TextMsg)
	{
		char text[256];
		buf.ReadString(text, sizeof(text));
		if (strnlen_s(text, sizeof(text)) > 0)
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
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable,       (LPVOID&)SVCPrint,       (LPVOID*)&SVC_Print_Process, 3);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VTable, (LPVOID&)SVCUserMessage, (LPVOID*)&SVC_UserMessage_Process, 3);
}

void CNetMessages_Detach()
{
	void* hkRestore = nullptr;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable,       (LPVOID)SVC_Print_Process,       (LPVOID*)&hkRestore, 3);
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_UserMessage_VTable, (LPVOID)SVC_UserMessage_Process, (LPVOID*)&hkRestore, 3);
}