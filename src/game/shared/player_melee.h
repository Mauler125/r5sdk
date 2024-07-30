#ifndef PLAYER_MELEE_H
#define PLAYER_MELEE_H

struct PlayerMelee_PlayerData
{
	void* __vftable;
	int meleeAttackParity;
	bool attackActive;
	bool attackRecoveryShouldBeQuick;
	bool isSprintAttack;
	char gap_f[1];
	float attackStartTime;
	int attackHitEntity;
	float attackHitEntityTime;
	float attackLastHitNonWorldEntity;
	int scriptedState;
	bool pendingMeleePress;
	char gap_25[3];
	Vector3D lungeBoost;
};

#endif // PLAYER_MELEE_H
