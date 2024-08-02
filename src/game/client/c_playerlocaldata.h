//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef C_PLAYERLOCALDATA_H
#define C_PLAYERLOCALDATA_H

#include "mathlib/vector.h"

struct C_PlayerLocalData
{
	void* _vftable;
	int m_nStepside;
	int m_nOldButtons;
	int m_iHideHUD;
	int m_nDuckTransitionTimeMsecs;
	int m_superJumpsUsed;
	bool m_jumpedOffRodeo;
	char gap_1d[3];
	float m_jumpPressTime;
	float m_jumpHoldTime;
	float m_jetpackActivateTime;
	float m_flSuitPower;
	float m_flSuitJumpPower;
	float m_flSuitGrapplePower;
	float m_flFallVelocity;
	float m_flStepSize;
	float m_airSlowMoFrac;
	int predictableFlags;
	int m_bitsActiveDevices;
	int m_forceStance;
	bool m_duckToggleOn;
	bool m_bDrawViewmodel;
	bool m_bAllowAutoMovement;
	char gap_53[305];
	Vector3D m_airMoveBlockPlanes[2];
	float m_airMoveBlockPlaneTime;
	int m_airMoveBlockPlaneCount;
	float m_queuedMeleePressTime;
	float m_queuedGrappleMeleeTime;
	char gap_1ac[1];
	bool m_disableMeleeUntilRelease;
	char gap_1ae[2];
	float m_meleePressTime;
	int m_meleeDisabledCounter;
	int m_meleeInputIndex;
	char gap_1bc[4];
	bool m_oneHandedWeaponUsage;
	bool m_prevOneHandedWeaponUsage;
	char gap_1c2[50];
	bool m_titanEmbarkEnabled;
	bool m_titanDisembarkEnabled;
	char gap_1f6[6];
	float m_playerAnimStationaryGoalFeetYaw;
	bool m_playerAnimJumping;
	char gap_201[3];
	float m_playerAnimJumpStartTime;
	bool m_playerAnimFirstJumpFrame;
	bool m_playerAnimDodging;
	char gap_20a[2];
	int m_playerAnimJumpActivity;
	bool m_playerAnimLanding;
	bool m_playerAnimShouldLand;
	char gap_212[2];
	float m_playerAnimLandStartTime;
	bool m_playerAnimInAirWalk;
	char gap_219[3];
	float m_playerAnimPrevFrameSequenceMotionYaw;
	int m_playerAnimMeleeParity;
	float m_playerAnimMeleeStartTime;
	Quaternion m_playerLocalGravityToWorldTransform;
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
	char gap_2e1[3];
	int m_useEnt;
};

#endif // C_PLAYERLOCALDATA_H
