//=============================================================================//
//
// Purpose: AI Shoot & Cover
//
//=============================================================================//
#ifndef AI_SHOOTCOVER_H
#define AI_SHOOTCOVER_H

#include "ai_hint.h"

struct RecentlyUsedCover
{
	CAI_Hint* hint;
	float nextUseTime;
};


struct ShootingCoverState
{
	CAI_Hint* lockedHint;
	int advancesLeftFromLockedHint;
	int burstCount;
	float burstStartTime;
	bool burstCompletedSuccessfully;
	bool burstUsingLeanAnims;
	char gap_16[2];
	float nextMoveToCoverRetryTime;
	float hasApproxLOSToEnemy_expireTime;
	float hasApproxLOSToEnemy_checkTime;
	char gap_24[4];
	RecentlyUsedCover recentlyUsedCover;
	char gap_38[64];
	int recentlyUsedCoverNewestIdx;
	int consecutiveIdleSchedCount;
	int numberOfIdlesBeforeSchedCheck;
	float onPathToShootingCoverFail_retryTime;
	float onTooCloseForCover_expireTime;
	float disregardAlternateThreat_expireTime;
	bool alternateThreatInUse;
	char gap_91[3];
	Vector3D alternateThreatPos;
	int alternateThreatState;
};
static_assert(sizeof(ShootingCoverState) == 0xA8);

#endif // AI_SHOOTCOVER_H
