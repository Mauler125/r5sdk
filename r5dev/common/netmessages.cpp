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
#include "engine/sys_utils.h"

bool SVC_Print::Process()
{
	if (this->m_szText)
	{
		DevMsg(eDLL_T::SERVER, this->m_szText);
	}

	return true; // Original just return true also.
}

void CNetMessages_Attach()
{
	auto SVCPrint = &SVC_Print::Process;
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable, (LPVOID&)SVCPrint, (LPVOID*)&SVC_Print_Process, 3);
}

void CNetMessages_Detach()
{
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable, (LPVOID)p_SVC_Print_Process, (LPVOID*)&SVC_Print_Process, 3);
}