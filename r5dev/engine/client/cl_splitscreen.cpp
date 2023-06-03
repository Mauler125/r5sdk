//=============================================================================
// Splitscreen manager
//
//=============================================================================
#include "cl_splitscreen.h"

CSplitScreen* g_pSplitScreenMgr;

#ifndef DEDICATED
CSetActiveSplitScreenPlayerGuard::CSetActiveSplitScreenPlayerGuard(char const* pchContext, int nLine, int slot)
{
	m_pchContext = pchContext;
	m_nLine = nLine;
	m_nSaveSlot = g_pSplitScreenMgr->SetActiveSplitScreenPlayerSlot(slot);
	m_bResolvable = g_pSplitScreenMgr->SetLocalPlayerIsResolvable(pchContext, nLine, true);
}

CSetActiveSplitScreenPlayerGuard::~CSetActiveSplitScreenPlayerGuard()
{
	g_pSplitScreenMgr->SetActiveSplitScreenPlayerSlot(m_nSaveSlot);
	g_pSplitScreenMgr->SetLocalPlayerIsResolvable(m_pchContext, m_nLine, m_bResolvable);
}
#endif
