#ifndef C_WEAPON_X
#define C_WEAPON_X

#include "..\c_baseanimating.h"
#include "game/shared/shared_activity.h"
#include "game/shared/r1/weapon_playerdata.h"
#include "game/shared/r1/smartammo.h"

class C_WeaponX : C_BaseAnimating
{
public:
	float GetZoomFOVInterpAmount(const float curTime) const;
	inline bool HasTargetZoomFOV() { return m_playerData.GetTargetZoomFOV() == *(float*)&m_modVars[3120]; }

private:
	EHANDLE m_weaponOwner;
	float m_lastPrimaryAttack;
	float m_nextReadyTime;
	float m_nextPrimaryAttackTime;
	float m_attackTimeThisFrame;
	int m_worldModelIndexOverride;
	int m_iWorldModelIndex;
	int m_holsterModelIndex;
	int m_droppedModelIndex;
	int m_nIdealSequence;
	sharedactivity_e m_IdealActivity;
	sharedactivity_e m_weaponActivity;
	int m_ActiveState;
	int m_ammoInClip;
	int m_ammoInStockpile;
	int m_lifetimeShots;
	float m_flTimeWeaponIdle;
	int m_weapState;
	bool m_allowedToUse;
	bool m_discarded;
	bool m_bInReload;
	char gap_123b[1];
	int m_forcedADS;
	char m_tossRelease;
	char gap_1241[3];
	sharedactivity_e m_customActivity;
	int m_customActivitySequence;
	EHANDLE m_customActivityOwner;
	float m_customActivityEndTime;
	char m_customActivityFlags;
	char gap_1255[3];
	WeaponPlayerData_Client m_playerData;
	bool m_smartAmmoEnable;
	char gap_1339[7];
	SmartAmmo_WeaponData_Client m_smartAmmo;
	char unk_pad[560];

	// TODO: reverse this properly and make this private !!!
public:
	char m_modVars[4432];
};

#endif // C_WEAPON_X
