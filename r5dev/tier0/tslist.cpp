//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "tier0/tslist.h"

//-----------------------------------------------------------------------------
// Purpose: alloc aligned memory
// Input  : nSize - 
//			nPad -
// Output : pointer to allocated aligned memory
//-----------------------------------------------------------------------------
void* CAlignedMemAlloc::Alloc(size_t nSize, size_t nAlignment)
{
	return g_pAlignedMemAlloc->m_pAllocCallback(nSize, nAlignment);
}

//-----------------------------------------------------------------------------
// Purpose: free aligned memory
// Input  : pMem - 
//-----------------------------------------------------------------------------
void CAlignedMemAlloc::Free(void* pMem)
{
	g_pAlignedMemAlloc->m_pFreeCallback(pMem);
}

CAlignedMemAlloc* g_pAlignedMemAlloc;
