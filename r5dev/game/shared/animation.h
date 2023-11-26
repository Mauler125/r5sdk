#ifndef ANIMATION_H
#define ANIMATION_H
#include "mathlib/vector.h"
#include "public/studio.h"
#include "public/ihandleentity.h"

class CAnimationLayer
{
	bool m_bSequenceFinished;
	char gap_1[3];
	int m_fFlags;
	int m_layerIndex;
	int m_modelIndex;
	float m_flKillRate;
	float m_flKillDelay;
	char m_nActivity[4];
	int m_nPriority;
	float m_flLastEventCheck;
	char gap_24[4];
	IHandleEntity* m_animationLayerOwner; // !TODO: CBaseEntity/C_BaseEntity?
};

struct PredictedAnimEventData
{
	char gap_0[8];
	float m_predictedAnimEventTimes[8];
	int m_predictedAnimEventIndices[8];
	int m_predictedAnimEventCount;
	int m_predictedAnimEventTarget;
	int m_predictedAnimEventSequence;
	int m_predictedAnimEventModel;
	float m_predictedAnimEventsReadyToFireTime;
	char gap_5C[4]; // <-- 64-BIT ALIGNMENT
};

struct AnimRelativeData
{
	char gap_0[8];
	Vector3D m_animInitialPos;
	Vector3D m_animInitialVel;
	Quaternion m_animInitialRot;
	Vector3D m_animInitialCorrectPos;
	Quaternion m_animInitialCorrectRot;
	Vector3D m_animEntityToRefOffset;
	Quaternion m_animEntityToRefRotation;
	float m_animBlendBeginTime;
	float m_animBlendEndTime;
	int m_animScriptSequence;
	int m_animScriptModel;
	bool m_animIgnoreParentRot;
	char gap_79[3];
	int m_animMotionMode;
	bool m_safePushMode;
	char gap_81[7]; // <-- 64-BIT ALIGNMENT
};

struct Player_AnimViewEntityData
{
	char gap_0[8];
	int animViewEntityHandle;
	float animViewEntityAngleLerpInDuration;
	float animViewEntityOriginLerpInDuration;
	float animViewEntityLerpOutDuration;
	bool animViewEntityStabilizePlayerEyeAngles;
	char gap_19[3];
	int animViewEntityThirdPersonCameraParity;
	int animViewEntityThirdPersonCameraAttachment[6];
	int animViewEntityNumThirdPersonCameraAttachments;
	bool animViewEntityThirdPersonCameraVisibilityChecks;
	bool animViewEntityDrawPlayer;
	char gap_3e[2];
	float fovTarget;
	float fovSmoothTime;
	int animViewEntityParity;
	int lastAnimViewEntityParity;
	int lastAnimViewEntityParityTick;
	Vector3D animViewEntityCameraPosition;
	Vector3D animViewEntityCameraAngles;
	float animViewEntityBlendStartTime;
	Vector3D animViewEntityBlendStartEyePosition;
	Vector3D animViewEntityBlendStartEyeAngles;
};


inline CMemory p_CStudioHdr__LookupSequence;
inline int(*v_CStudioHdr__LookupSequence)(CStudioHdr* pStudio, const char* pszName);

///////////////////////////////////////////////////////////////////////////////
class VAnimation : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CStudioHdr::LookupSequence", p_CStudioHdr__LookupSequence.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CStudioHdr__LookupSequence = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 89 ?? ?? ?? ??");
		v_CStudioHdr__LookupSequence = p_CStudioHdr__LookupSequence.RCast<int(*)(CStudioHdr*, const char*)>(); /*40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 89 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // ANIMATION_H
