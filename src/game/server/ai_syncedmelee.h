//=============================================================================//
//
// Purpose: AI Synced Melee
//
//=============================================================================//
#ifndef AI_SYNCEDMELEE_H
#define AI_SYNCEDMELEE_H

struct AISyncedMeleeState
{
	const char* desiredMelee;
	float additionalYaw;
	int meleePartner;
	bool ragdollOnInterrupt;
	bool continueAnimToFinish;
	char gap_12[6];
	char debouncedMelees[48];
	float pressToInitiate_debounceExpireTime;
	Vector3D pressToInitiate_pos;
	float pressToInitiate_dist;
};
static_assert(sizeof(AISyncedMeleeState) == 0x60);

#endif // AI_SYNCEDMELEE_H
