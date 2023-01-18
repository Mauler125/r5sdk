//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERLOCALDATA_H
#define PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif
#include "mathlib/mathlib.h"
#include "game/shared/playernet_vars.h"


//-----------------------------------------------------------------------------
// Purpose: Player specific data ( sent only to local player, too )
//-----------------------------------------------------------------------------
class CPlayerLocalData
{
	char gap_0[8];
	int m_iHideHUD;
	Vector3D m_vecOverViewpoint;
	bool m_duckToggleOn;
	char gap_19[3];
	int m_forceStance;
	int m_nDuckTransitionTimeMsecs;
	int m_superJumpsUsed;
	bool m_jumpedOffRodeo;
	char gap_29[3];
	float m_jumpPressTime;
	float m_jumpHoldTime;
	float m_jetpackActivateTime;
	float m_flSuitPower;
	float m_flSuitJumpPower;
	float m_flSuitGrapplePower;
	int m_nStepside;
	float m_flFallVelocity;
	int m_nOldButtons;
	float m_oldForwardMove;
	char gap_54[4];
	void* m_pOldSkyCamera;
	float m_accelScale;
	float m_powerRegenRateScale;
	float m_dodgePowerDelayScale;
	bool m_bDrawViewmodel;
	char gap_6d[3];
	float m_flStepSize;
	bool m_bAllowAutoMovement;
	char gap_75[3];
	float m_airSlowMoFrac;
	int predictableFlags;
	int m_bitsActiveDevices;
	int m_hSkyCamera;
	sky3dparams_t m_skybox3d;
	CFogParams m_fog;
	audioparams_t m_audio;
	float m_animNearZ;
	Vector3D m_airMoveBlockPlanes[2];
	float m_airMoveBlockPlaneTime;
	int m_airMoveBlockPlaneCount;
	float m_queuedMeleePressTime;
	float m_queuedGrappleMeleeTime;
	bool m_queuedMeleeAttackAnimEvent;
	bool m_disableMeleeUntilRelease;
	char gap_1e6[2];
	float m_meleePressTime;
	int m_meleeDisabledCounter;
	int m_meleeInputIndex;
	int lastAttacker;
	int attackedCount;
	int m_trackedChildProjectileCount;
	bool m_oneHandedWeaponUsage;
	bool m_prevOneHandedWeaponUsage;
	char gap_202[2];
	float m_flCockpitEntryTime;
	float m_ejectStartTime;
	float m_disembarkStartTime;
	float m_hotDropImpactTime;
	float m_outOfBoundsDeadTime;
	int m_objectiveIndex;
	int m_objectiveEntity;
	float m_objectiveEndTime;
	int m_cinematicEventFlags;
	bool m_forcedDialogueOnly;
	char gap_229[3];
	float m_titanBuildTime;
	float m_titanBubbleShieldTime;
	bool m_titanEmbarkEnabled;
	bool m_titanDisembarkEnabled;
	char gap_236[2];
	int m_voicePackIndex;
	float m_playerAnimStationaryGoalFeetYaw;
	bool m_playerAnimJumping;
	char gap_241[3];
	float m_playerAnimJumpStartTime;
	bool m_playerAnimFirstJumpFrame;
	bool m_playerAnimDodging;
	char gap_24a[2];
	int m_playerAnimJumpActivity;
	bool m_playerAnimLanding;
	bool m_playerAnimShouldLand;
	char gap_252[2];
	float m_playerAnimLandStartTime;
	bool m_playerAnimInAirWalk;
	char gap_259[3];
	float m_playerAnimPrevFrameSequenceMotionYaw;
	int m_playerAnimMeleeParity;
	float m_playerAnimMeleeStartTime;
	matrix3x4_t m_playerLocalGravityToWorldTransform;
	Quaternion m_playerLocalGravityBlendStartRotation;
	Quaternion m_playerLocalGravityBlendEndRotation;
	Vector3D m_playerLocalGravityBlendEndDirection;
	float m_playerLocalGravityBlendStartTime;
	float m_playerLocalGravityBlendEndTime;
	float m_playerLocalGravityBlendStrength;
	float m_playerLocalGravityStrength;
	int m_playerLocalGravityType;
	Vector3D m_playerLocalGravityPoint;
	Vector3D m_playerLocalGravityLineStart;
	Vector3D m_playerLocalGravityLineEnd;
	int m_playerLocalGravityEntity;
	int m_playerLocalGravityLineStartEntity;
	int m_playerLocalGravityLineEndEntity;
	float m_playerFloatLookStartTime;
	float m_playerFloatLookEndTime;
	float m_wallrunLatestFloorHeight;
	Vector3D m_groundNormal;
	bool m_continuousUseBlocked;
};

#endif // PLAYERLOCALDATA_H
