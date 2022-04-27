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

bool HSVC_Print_Process(SVC_Print* thisptr)
{
	if (thisptr->m_szText)
	{
		DevMsg(eDLL_T::SERVER, thisptr->m_szText);
	}

	return true; // Original just return true also.
}

void CNetMessages_Attach()
{
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable, (LPVOID)HSVC_Print_Process, (LPVOID*)&SVC_Print_Process, 3);
}

void CNetMessages_Detach()
{
	CMemory::HookVirtualMethod((uintptr_t)g_pSVC_Print_VTable, (LPVOID)p_SVC_Print_Process, (LPVOID*)&SVC_Print_Process, 3);
}