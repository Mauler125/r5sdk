//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Base NPC character with AI
//
//=============================================================================//
#ifndef AI_BASENPC_H
#define AI_BASENPC_H

#ifdef _WIN32
#pragma once
#endif

#include "game/shared/shared_activity.h"
#include "game/shared/simtimer.h"
#include "game/shared/status_effect.h"
#include "basecombatcharacter.h"
#include "ai_movetypes.h"
#include "ai_schedule.h"
#include "ai_task.h"
#include "ai_senses.h"
#include "ai_motor.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_localnavigator.h"
#include "ai_memory.h"
#include "ai_utility.h"
#include "ai_squad.h"
#include "ai_tacticalservices.h"
#include "ai_moveshoot.h"
#include "ai_battlelocation.h"
#include "ai_hint.h"
#include "ai_syncedmelee.h"
#include "ai_shootcover.h"

//=============================================================================
// Purpose: Some bridges a little more complicated to allow behavior to see 
//			what base class would do or control order in which it's done
//=============================================================================

abstract_class IAI_BehaviorBridge
{
public:
	#define AI_GENERATE_BRIDGE_INTERFACE
	//#include "ai_behavior_template.h"

	// Non-standard bridge methods
	virtual void 		 BehaviorBridge_GatherConditions() {}
	// TODO: confirm if this interface is correct for R5!
	//virtual int 		 BehaviorBridge_SelectSchedule() { return 0; }
	//virtual int 		 BehaviorBridge_TranslateSchedule(int scheduleType) { return 0; }
	//virtual float		 BehaviorBridge_GetJumpGravity() const { return 0; }
	//virtual bool		 BehaviorBridge_IsJumpLegal(const Vector3D & startPos, const Vector3D& apex, const Vector3D& endPos, float maxUp, float maxDown, float maxDist) const { return 0; }
	//virtual bool		 BehaviorBridge_MovementCost(int moveType, const Vector3D& vecStart, const Vector3D& vecEnd, float* pCost) { return 0; }
};

//=============================================================================
//
// Types used by CAI_BaseNPC
//
//=============================================================================

struct AIScheduleState_t
{
	int                  iCurTask;
	TaskStatus_e         fTaskStatus;
	float                timeStarted;
	float                timeCurTaskStarted;
	int                  failedSchedID;
	char gap_14[4];
	AI_TaskFailureCode_t taskFailureCode;
	int                  iTaskInterrupt;
	bool                 bTaskRanAutomovement;
	bool                 bTaskRequestRunTillWait;
	bool                 doYawUpdatesForThisDecisionInterval;
	bool                 doYawUpdatesDuringThisTask;
	bool                 updateIdealYawToEnemyDuringThisTask;
	bool                 bScheduleWasInterrupted;
	bool                 bStopRunTasks;

	//DECLARE_SIMPLE_DATADESC();
};

//=============================================================================
//
//	class CAI_BaseNPC
//
//=============================================================================

class CAI_BaseNPC : public CBaseCombatCharacter,
                    public CAI_DefMovementSink,
                    public IAI_BehaviorBridge
{
public:

private:
	int m_threadedPostProcessJob;
	bool m_bDoPostProcess;
	bool m_bCustomEnemySearch;
	bool m_bPlayerSpottable;
	bool m_bAddedToSpottableList;
	CAI_Schedule* m_pPrevSchedule;
	CAI_Schedule* m_pSchedule;
	int m_defaultScriptScheduleID;
	char gap_598c[4];
	AIScheduleState_t m_ScheduleState;
	int m_failSchedule;
	bool m_bDoPostRestoreRefindPath;
	bool m_bDoScheduleChangeSignal;
	bool m_spawned;
	bool m_bUsingStandardThinkTime;
	float m_flLastRealThinkTime;
	float m_flLastThinkDuration;
	int m_iTickBlocked;
	float m_dangerousClusterConditionAllowedAtTime;
	int m_poseAim_Pitch;
	int m_poseAim_Yaw;
	int m_poseMove_Yaw;
	int m_poseMove_Lean;
	float m_offsetOfCurrentAimFromDesiredAim_Cos;
	bool m_haveDesiredAimDir;
	bool m_sequenceHasAimEvent;
	bool m_sequenceHasAimPoseParams;
	char gap_59ef[1];
	int m_lastAimSequence;
	int m_missPlayerLastWindow;
	Vector3D m_missPlayerLastOffset;
	char gap_5a04[4];
	void* m_pPrimaryBehavior;
	char m_Behaviors[32];
	bool m_bCalledBehaviorSelectSchedule;
	char gap_5a31[3];
	char m_Conditions[12];
	char m_CustomInterruptConditions[12];
	bool m_bForceConditionsGather;
	bool m_bConditionsGathered;
	bool m_bInterruptableByScript;
	char gap_5a4f[1];
	int m_movingGroundEnt;
	Vector3D m_groundRelativePos;
	Vector3D m_groundRelativeAngles;
	int m_NPCState;
	int m_NPCPrevState;
	int m_lastSchedSelectorState;
	float m_flLastSchedSelectChangeTime;
	float m_flLastStateChangeTime;
	float m_thinkInterval;
	int m_IdealNPCState;
	int m_Efficiency;
	int m_MoveEfficiency;
	float m_flNextDecisionTime;
	__int16 m_searchPathID;
	bool m_bDefenseActive;
	bool m_bAlwaysAlert;
	bool m_scriptedAnimForceInterrupt;
	bool m_bWakeSquad;
	char gap_5a9a[2];
	float m_flWakeRadius;
	int m_nWakeTick;
	int m_SleepState;
	int m_SleepFlags;
	char m_translatedActivity[4];
	sharedactivity_e m_IdealActivity;
	int m_nIdealSequence;
	sharedactivity_e m_IdealTranslatedActivity;
	sharedactivity_e m_IdealWeaponActivity;
	bool m_idealSequenceAlreadyResolved;
	char gap_5ac1[3];
	int m_seqFinishedInSolid;
	int m_prevSeqWeight;
	char gap_5acc[4];
	char m_activeActModifiers[80];
	int m_scriptIdleSequence;
	int m_scriptAttackSequence;
	int m_scriptDeathActivity;
	bool m_requestSpecialRangeAttack;
	char gap_5b2d[3];
	int m_specialRangeAttackCount;
	char gap_5b34[4];
	__int64 m_aiSettings;
	int m_aiSettingsIndex;
	int m_subclass;
	CAI_Senses* m_pSenses;
	CSound m_pLockedBestSound;
	Vector3D m_aimDir;
	bool m_aimDirValid;
	char gap_5b85[3];
	float m_weaponBlockedTimer;
	float m_weaponBlockedTimeOut;
	int m_moveDirection;
	int m_hEnemy;
	int m_hEnemySecondary;
	float m_distToEnemyLKP_AdjustForHeightDiff;
	float m_distToEnemyLKP;
	float m_distToEnemyLKPCenterToCenter;
	char m_updateSquadEnemyQueue[32];
	CStopwatch m_notifyEnemyToSquadTimer;
	float m_notifyEnemyToTeamTime;
	CRandStopwatch m_GiveUpOnDeadEnemyTimer;
	CSimpleSimTimer m_chooseEnemySanityTimer;
	int m_EnemiesSerialNumber;
	CSimpleSimTimer m_UpdateEnemyPosTimer;
	bool m_ForceUpdateEnemyPos;
	char gap_5bf5[3];
	int m_afCapability;
	int m_flags;
	bool m_chanceToHitIsEnabled;
	char gap_5c01[3];
	float m_flMoveWaitFinished;
	char m_UnreachableEnts[32];
	CAI_Navigator* m_pNavigator;
	CAI_LocalNavigator* m_pLocalNavigator;
	CAI_Pathfinder* m_pPathfinder;
	CAI_MoveProbe* m_pMoveProbe;
	CAI_Motor* m_pMotor;
	int m_hGoalEnt;
	float m_flTimeLastMovement;
	float m_longJumpCheckTime;
	Vector3D m_prevGroundNormal;
	CSimpleSimTimer m_CheckOnGroundTimer;
	Vector3D m_vDefaultLocalEyeOffset;
	float m_flNextEyeLookTime;
	float m_flEyeIntegRate;
	Vector3D m_vEyeLookTarget;
	Vector3D m_vCurEyeTarget;
	float m_flHeadYaw;
	float m_flHeadPitch;
	int m_animRefAdjustThinkCount;
	Vector3D m_animRefAdjustPerThink;
	bool m_animRefDidAdjust;
	bool m_animParentedOnPlay;
	char gap_5cb2[2];
	int m_scriptAnimSavedCollisionGroup;
	int m_scriptAnimSavedFlags;
	int m_scriptAnimStartPolyRef;
	char m_enemyChangeScriptCallback[16];
	CAI_Enemies* m_pEnemies;
	int m_afMemory;
	int m_hEnemyOccluder;
	int m_hScriptEnemy;
	int m_hNearestVisibleFriendlyPlayer;
	int m_lastDamageFlags;
	int m_lastDamageType;
	float m_strafeDodgeDamage;
	float m_lastLightPainTime;
	float m_lastHeavyPainTime;
	float m_flSumDamage;
	float m_flLastDamageTime;
	float m_flLastSawPlayerTime;
	float m_flLastAttackTime;
	float m_flAlertEventTime;
	float m_flNextRangeAttackSecondary;
	float m_flNextMeleeAllowTime;
	float m_flNextMeleeAltAllowTime;
	int m_meleeComboCount;
	bool m_bIgnoreUnseenEnemies;
	char gap_5d21[7];
	CAI_ShotRegulator m_ShotRegulator;
	AISyncedMeleeState m_syncedMelee;
	CAI_Squad* m_pSquad;
	string_t m_SquadName;
	int m_iMySquadSlot;
	int m_squadAssignment;
	float m_squadAssignedRange;
	float m_squadAssignedNodeStartUseTime;
	int m_squadAssignedNode;
	int m_lockedNode;
	int m_currentWeaponBarrel;
	bool m_bAutoSquad;
	bool m_bDidDeathCleanUp;
	bool m_bOptionalSprint;
	bool m_bClearNodeOnScheduleFail;
	bool m_bRunningFromEnemy;
	char m_runFromEnemyRetry;
	char m_disableArrivalOnce;
	char gap_5e03[5];
	CAI_TacticalServices* m_pTacticalServices;
	float m_scriptAimingPrecisionAdjustment;
	float m_flWaitFinished;
	float m_flNextFlinchTime;
	float m_flNextCheckFaceEnemyTime;
	CAI_MoveAndShootOverlay m_moveAndShootOverlay;
	bool m_forceShootOverlay;
	bool m_weaponTemporarilySwitchedByAnim;
	bool m_strafeDir;
	char gap_5e43[1];
	int m_strafeCount;
	float m_flSavePositionTime;
	Vector3D m_vSavePosition;
	Vector3D m_vInterruptSavePosition;
	Vector3D m_vLastAutoMoveDelta;
	Vector3D m_autoMoveAdjust_originalSpace;
	Vector3D m_vLastPatrolDir;
	CAI_Hint* m_pHintNode;
	int m_pSafeHintNode;
	int m_safeHintType;
	float m_flDistTooFar;
	float m_flDistTooClose;
	float m_minEngagementDistVsWeak;
	float m_maxEngagementDistVsWeak;
	float m_minEngagementDistVsStrong;
	float m_maxEngagementDistVsStrong;
	float m_dangerousAreaRadius;
	float m_maxEnemyDistOverride;
	float m_maxEnemyDistHeavyArmorOverride;
	float m_minEnemyDist;
	float m_disengageEnemyDist;
	char gap_5ec4[4];
	string_t m_spawnEquipment[2];
	void* m_grenadeWeapon; // todo: reverse and add class
	string_t m_grenadeWeaponName;
	Vector3D m_lastValidGrenadeThrowOrigin;
	float m_throwGrenadeAllowedTime;
	float m_rangeAttackTwitchAllowedTime;
	float m_smartAmmoLockTime;
	int m_smartAmmoLocks;
	int m_smartAmmoWeapon;
	int m_meleeWeapon;
	int m_reactFriendlyChance;
	int m_reactBulletChance;
	int m_reactChance;
	float m_lastReactTime;
	float m_dangerousAreaReactionTime;
	int m_MovementCollisionGroup;
	float m_moveDeflectionSearchRadius;
	float m_moveDeflectionDebounceExpireTime;
	char gap_5f2c[4];
	ShootingCoverState shootingCover;
	BattleLocationState m_battleLocation;
	char m_OnDamaged[40];
	char m_OnFoundEnemy[40];
	char m_OnSeeEnemy[40];
	char m_OnCantSeeEnemy[40];
	char m_OnNoticePotentialEnemy[40];
	char m_OnGainEnemyLOS[40];
	char m_OnLostEnemyLOS[40];
	char m_OnLostEnemy[40];
	char m_OnFoundPlayer[40];
	char m_OnLostPlayerLOS[40];
	char m_OnLostPlayer[40];
	char m_OnHearPlayer[40];
	char m_OnHearCombat[40];
	char m_OnSleep[40];
	char m_OnWake[40];
	char m_OnStateChange[40];
	char m_OnFailedToPath[40];
	char m_OnEnterGoalRadius[40];
	char m_OnFinishedAssault[40];
	char m_OnSawCorpse[40];
	char m_OnGiveWeapon[40];
	char m_OnTakeWeapon[40];
	char m_OnSpecialAttack[40];
	char m_OnScheduleChange[40];
	char m_OnSyncedMeleeBegin[40];
	char m_queuedOutput[32];
	char m_pInitializedPhysicsObject[8];
	bool m_fIsUsingSmallHull;
	bool m_bHadMovingGround;
	bool m_bCheckContacts;
	bool m_bAllowShoot;
	bool m_bActiveForSmartAmmo;
	bool m_bEnemyValidSmartAmmoTarget;
	bool m_bAllowBlockBullets;
	bool m_blockingBullets;
	int m_reactingSurprisedReason;
	bool m_desireCrouch;
	bool m_isCrouching;
	bool m_bAutoMovementBlocked;
	bool m_bAllowPushDuringAnim;
	float m_desireStandOverrideExpireTime;
	char gap_6404[4];
	char m_schedSelectorHistory[40];
	int m_behaviorSelectorID;
	char gap_6434[4];
	const char* m_failText;
	const char* m_interruptText;
	CAI_Schedule* m_failedSchedule;
	CAI_Schedule* m_interuptSchedule;
	string_t m_debugSchedSelectorName;
	float m_lastTaskTextMsgTime;
	float m_flAccuracyMultiplier;
	float m_flAccuracyMultiplierForEnemy;
	float m_LastMissFastPlayerTime;
	float m_LastSuppressionTime;
	float m_LastShootAccuracy;
	int m_TotalShots;
	int m_TotalHits;
	float m_flSoundWaitTime;
	int m_nSoundPriority;
	float m_lastTauntTime;
	float m_freezeTime;
	float m_freezeCycle;
	float m_freezePlaybackRate;
	int m_prevShieldHealth;
	int m_healthEvalMultiplier;
	float m_aiMovementSpeed;
	bool m_aiSprinting;
	char gap_64a5[3];
	int m_aiNetworkFlags;
	bool m_isHologram;
	char gap_64ad[3];
	int m_fireteamSlotIndex;
	char gap_64b4[4];
	StatusEffectTimedData m_statusEffectsTimedNPCNV;
	char gap_64d0[48];
	StatusEffectEndlessData m_statusEffectsEndlessNPCNV;
	char gap_6510[80];
	char m_title[32];
	bool m_tethered;
	char gap_6581[3];
	Quaternion m_localGravityRotation;
	matrix3x4_t m_localGravityToWorldTransform;
	int m_nAITraceMask;
	float m_flBoostSpeed;
	float m_blockPeriodStartTime;
	int m_blockBulletCount;
	float m_dodgePeriodStartTime;
	float m_lastDodgeTime;
	int m_dodgeCount;
	int m_dodgeProjectile;
	Vector3D m_dodgeFromPos;
	char m_dangerousArea[8];
	float m_dangerousAreaDebounceExpireTime;
	bool m_fallingState;
	bool m_grappled;
	bool m_grappleEndFloating;
	char gap_65ff[1];
	int m_grappleRestoreMoveType;
	char gap_6604[4];
	char m_threadRandom[64];
};

static_assert(sizeof(CAI_BaseNPC) == 0x6648);

#endif // AI_BASENPC_H
