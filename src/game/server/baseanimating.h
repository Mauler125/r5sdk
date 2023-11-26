//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef BASEANIMATING_H
#define BASEANIMATING_H
#ifdef _WIN32
#pragma once
#endif
#include "baseentity.h"
#include "public/studio.h"
#include "game/shared/animation.h"


class CBaseAnimating : public CBaseEntity
{
public:
	void DrawServerHitboxes(float duration = 0.0f);

	void LockStudioHdr(void);
	void HitboxToWorldTransforms(uint32_t iBone, matrix3x4_t* transforms);

	CStudioHdr*			GetModelPtr(void);
	float				GetModelScale() const { return m_flModelScale; }

protected:
	void* __vftable;
	bool m_markedForServerInterpolation;
	bool m_animRemoveFromServerInterpolationNextFrame;
	char gap_b12[2];
	float m_flGroundSpeed;
	float m_flLastEventCheck;
	int m_nForceBone;
	Vector3D m_vecForce;
	int m_nSkin;
	__int16 m_skinMod;
	char gap_b32[2];
	int m_nBody;
	int m_camoIndex;
	int m_decalIndex;
	int m_nHitboxSet;
	float m_flModelScale;
	int m_nRagdollImpactFXTableId;
	float m_flSkyScaleStartTime;
	float m_flSkyScaleEndTime;
	float m_flSkyScaleStartValue;
	float m_flSkyScaleEndValue;
	char gap_b5c[4];
	char m_SequenceTransitioner[368];
	float m_flIKGroundContactTime;
	float m_flIKGroundMinHeight;
	float m_flIKGroundMaxHeight;
	float m_flEstIkFloor;
	float m_flEstIkOffset;
	char gap_ce4[4];
	char m_pIk[8];
	int m_ikPrevSequence;
	bool m_bSequenceFinished;
	bool m_bSequenceLooped;
	bool m_bSequenceLoops;
	bool m_continueAnimatingAfterRagdoll;
	float m_lockedAnimDeltaYaw;
	bool m_threadedBoneSetup;
	bool m_settingUpBones;
	char gap_cfe[2];
	float m_flDissolveStartTime;
	int m_baseAnimatingActivity;
	float m_flPoseParameter[12];
	bool m_poseParameterOverTimeActive;
	char gap_d39[3];
	float m_poseParameterGoalValue[12];
	float m_poseParameterEndTime[12];
	float m_lastTimeSetPoseParametersSameAs;
	bool m_bClientSideAnimation;
	bool m_bReallyClientSideAnimation;
	char gap_da2[2];
	int m_nNewSequenceParity;
	int m_nResetEventsParity;
	char gap_dac[4];
	__int64 m_boneCacheHandle;
	__int16 m_fBoneCacheFlags; // END CBASEANIMATING
	char gap_dba[2];
	int m_animNetworkFlags;
	bool m_animActive;
	bool m_animCollisionEnabled;
	bool m_animPlantingEnabled;
	bool m_animInitialCorrection;
	bool m_animWaitingForCleanup;
	char gap_dc5[3];
	int m_animWaitingForCleanupTime;
	char gap_dcc[4];
	__int64 m_recordedAnim;
	int m_recordedAnimIndex;
	int m_recordedAnimCachedFrameIndex;
	float m_recordedAnimPlaybackRate;
	float m_recordedAnimPlaybackTime;
	matrix3x4_t m_recordedAnimTransform;
	int m_recordedAnimPlaybackEnt;
	float m_recordedAnimBlendTime;
	Vector3D m_recordedAnimBlendOffset;
	Vector3D m_recordedAnimBlendAngles;
	AnimRelativeData m_animRelativeData;
	int m_syncingWithEntity;
	char gap_ec4[4];
	PredictedAnimEventData m_predictedAnimEventData;
	int m_animRefEntityAttachmentIndex;
	int m_fireAttachmentSmartAmmoIndex;
	int m_fireAttachmentChestFocusIndex;
	int m_fireAttachmentModelIndex;
	char m_keyHitboxes[160];
	CStudioHdr* m_pStudioHdr;
	int m_animSequence;
	float m_animCycle;
	int m_animModelIndex;
	float m_animStartTime;
	float m_animStartCycle;
	float m_animPlaybackRate;
	bool m_animFrozen;
	char gap_ff9[7];
	__int64 m_createdProp[8];
	int m_numCreatedProps;
	char gap_1044[4];
	__int64 m_currentFramePropEvents[16];
	int m_numCurrentFramePropEvents;
	char gap_10cc[4];
	__int64 m_activeScriptAnimWindows[8];
	int m_numActiveScriptAnimWindows;
	char gap_1114[4];
	__int64 m_currentFrameWindowEvents[16];
	int m_numCurrentFrameWindowEvents;
	char gap_119c[4];
	__int64 m_AnimSyncScriptProps[8];
	int m_numAnimSyncScriptProps;
};

inline CMemory p_CBaseAnimating__LockStudioHdr;
inline CBaseAnimating*(*v_CBaseAnimating__LockStudioHdr)(CBaseAnimating* thisp);

///////////////////////////////////////////////////////////////////////////////
class VBaseAnimating : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CBaseAnimating::LockStudioHdr", p_CBaseAnimating__LockStudioHdr.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CBaseAnimating__LockStudioHdr = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 41 56 48 83 EC 20 0F BF 41 58");
		v_CBaseAnimating__LockStudioHdr = p_CBaseAnimating__LockStudioHdr.RCast<CBaseAnimating* (*)(CBaseAnimating* thisp)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // BASEANIMATING_H
