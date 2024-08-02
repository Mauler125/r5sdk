//=============================================================================//
//
// Purpose: AI system utilities
//
//=============================================================================//

#ifndef AI_UTILITY_H
#define AI_UTILITY_H

class CAI_BaseNPC;

//-----------------------------------------------------------------------------
//
// CAI_ShotRegulator
//
// Purpose: Assists in creating non-constant bursty shooting style
//
//-----------------------------------------------------------------------------
class CAI_ShotRegulator
{
private:
	int m_burstIndex;
	int m_nBurstShotsRemaining;
	int m_nMinBurstShots;
	int m_nMaxBurstShots;
	float m_flNextShotTime;
	float m_flFireRateDelay;
	float m_flMinRestInterval;
	float m_flMaxRestInterval;
	float m_flLastPreFireDelayTime;
	bool m_bInRestInterval;
	bool m_bFirstShot;
	bool m_haveRolledForMiss;
	bool m_rolledAMiss;
	float m_missDeltaRight;
	float m_missDeltaUp;
	float m_missDeltaForward;
	int m_missExtraAAEFlags;
	bool m_haveSelectedBurstTarget;
	char gap_39[7];
	CAI_BaseNPC* m_ai;
};
static_assert(sizeof(CAI_ShotRegulator) == 0x48);

#endif // AI_UTILITY_H
