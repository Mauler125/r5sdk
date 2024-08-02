//=============================================================================//
//
// Purpose: AI Battle Location
//
//=============================================================================//
#ifndef AI_BATTLELOCATION_H
#define AI_BATTLELOCATION_H

struct BattleLocationState
{
	int occupiedCell;
	float searchFailedExpireTime;
};
static_assert(sizeof(BattleLocationState) == 8);

#endif // AI_BATTLELOCATION_H
