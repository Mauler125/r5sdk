//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "tier0/tslist.h"

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : *pAllocCallback - 
//          *pFreeCallback - 
//-----------------------------------------------------------------------------
CAlignedMemAlloc::CAlignedMemAlloc(FnAlloc_t pAllocCallback, FnFree_t pFreeCallback)
{
	m_pAllocCallback = pAllocCallback;
	m_pFreeCallback = pFreeCallback;
}

CAlignedMemAlloc* g_pAlignedMemAlloc;
