//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "engine/clockdriftmgr.h"

void CClockDriftMgr::Clear()
{
	m_nClientTick = 0;
	m_nServerTick = 0;
	m_iCurClockOffset = 0;
	memset(m_ClockOffsets, 0, sizeof(m_ClockOffsets));
}

float CClockDriftMgr::GetCurrentClockDifference() const
{
	// Note: this could be optimized a little by updating it each time we add
	// a sample (subtract the old value from the total and add the new one in).
	float total = 0;
	for (int i = 0; i < NUM_CLOCKDRIFT_SAMPLES; i++)
		total += m_ClockOffsets[i];

	return total / NUM_CLOCKDRIFT_SAMPLES;
}
