//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TAKEDAMAGEINFO_H
#define TAKEDAMAGEINFO_H
#ifdef _WIN32
#pragma once
#endif
#include "mathlib/mathlib.h"


// Used to initialize m_flBaseDamage to something that we know pretty much for sure
// hasn't been modified by a user. 
#define BASEDAMAGE_NOT_SPECIFIED	FLT_MAX // !TODO: check if this is correct for r5.

struct ScriptOriginatedDamageInfo
{
	int m_scriptDamageType;
	int m_damageSourceIdentifier;
};

/* 1426 */
class CTakeDamageInfo
{
	float m_vecDamageForceMagnitude;
	Vector3D m_vecDamageForceDirection;
	Vector3D m_vecDamagePosition;
	Vector3D m_vecReportedPosition;
	int m_hInflictor;
	int m_hAttacker;
	int m_hWeapon;
	__int16 m_hWeaponFileInfo;
	bool m_forceKill;
	char gap_37[1];
	float m_flDamage;
	float m_damageCriticalScale;
	float m_damageShieldScale;
	float m_flMaxDamage;
	float m_flHeavyArmorDamageScale;
	int m_bitsDamageType;
	float m_flRadius;
	int m_hitGroup;
	int m_hitBox;
	ScriptOriginatedDamageInfo m_scriptDamageInfo;
	int m_deathPackage;
	float m_distanceFromAttackOrigin;
	float m_distanceFromExplosionCenter;
	bool m_doDeathForce;
	char gap_71[3];
	int m_damageFlags;
	int m_projectilePredictableId;
	int m_flinchDirection;
};

#endif // TAKEDAMAGEINFO_H