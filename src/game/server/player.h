//===========================================================================//
//
// Purpose: 
//
//===========================================================================//

#ifndef PLAYER_H
#define PLAYER_H

#include "baseentity.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector4d.h"
#include "mathlib/bitvec.h"
#include "public/studio.h"
#include "public/playerstate.h"

#include "game/shared/animation.h"
#include "game/shared/status_effect.h"
#include "game/shared/takedamageinfo.h"
#include "game/shared/usercmd.h"
#include "game/shared/imovehelper.h"
#include "game/shared/player_viewoffset.h"
#include "game/shared/player_melee.h"
#include "game/shared/r1/grapple.h"

#include "playerlocaldata.h"
#include "basecombatcharacter.h"

enum PlayerConnectedState
{
	PlayerConnected,
	PlayerDisconnecting,
	PlayerDisconnected,
};


// TODO: Move to separate header file!!
struct ThirdPersonViewData
{
	void* __vftable;
	Vector3D m_thirdPersonEntViewOffset;
	bool m_thirdPersonEntShouldViewAnglesFollowThirdPersonEnt;
	bool m_thirdPersonEntPitchIsFreelook;
	bool m_thirdPersonEntYawIsFreelook;
	bool m_thirdPersonEntUseFixedDist;
	bool m_thirdPersonEntFixedClientOnly;
	bool m_thirdPersonEntPushedInByGeo;
	bool m_thirdPersonEntDrawViewmodel;
	char gap_1b[1];
	float m_thirdPersonEntBlendTotalDuration;
	float m_thirdPersonEntBlendEaseInDuration;
	float m_thirdPersonEntBlendEaseOutDuration;
	float m_thirdPersonEntFixedPitch;
	float m_thirdPersonEntFixedYaw;
	float m_thirdPersonEntFixedDist;
	float m_thirdPersonEntFixedHeight;
	float m_thirdPersonEntMinYaw;
	float m_thirdPersonEntMaxYaw;
	float m_thirdPersonEntMinPitch;
	float m_thirdPersonEntMaxPitch;
	float m_thirdPersonEntSpringToCenterRate;
	float m_thirdPersonEntLookaheadLowerEntSpeed;
	float m_thirdPersonEntLookaheadUpperEntSpeed;
	float m_thirdPersonEntLookaheadMaxAngle;
	float m_thirdPersonEntLookaheadLerpAheadRate;
	float m_thirdPersonEntLookaheadLerpToCenterRate;
};
struct PlayerZiplineData
{
	void* __vftable;
	bool m_ziplineReenableWeapons;
	char gap_9[3];
	float m_mountingZiplineDuration;
	float m_mountingZiplineAlpha;
	float m_ziplineStartTime;
	float m_ziplineEndTime;
	Vector3D m_mountingZiplineSourcePosition;
	Vector3D m_mountingZiplineSourceVelocity;
	Vector3D m_mountingZiplineTargetPosition;
	Vector3D m_ziplineUsePosition;
	float m_slidingZiplineAlpha;
	Vector3D m_lastMoveDir2D;
	bool m_ziplineReverse;
};
struct CurrentData_Player
{
	void* __vftable;
	float m_flHullHeight;
	float m_traversalAnimProgress;
	float m_sprintTiltFrac;
	Vector3D m_angEyeAngles;
	int m_ammoPoolCount[8];
};
struct CurrentData_LocalPlayer
{
	void* __vftable;
	Vector3D m_viewConeAngleMin;
	Vector3D m_viewConeAngleMax;
	Vector3D m_stepSmoothingOffset;
	Vector3D m_vecPunchBase_Angle;
	Vector3D m_vecPunchBase_AngleVel;
	Vector3D m_vecPunchWeapon_Angle;
	Vector3D m_vecPunchWeapon_AngleVel;
	int m_pushedFixedPointOffset[3];
	int m_pushedFixedPointOffsetReplayCompensated[3];
	Quaternion m_localGravityRotation;
};
struct MatchMetrics
{
	bool hotDropped;
	bool relinquished;
	char gap[2];
	int allPings;
	int locationPings;
	int enemyPings;
	int shotsFired;
	int shotsHit;
	int level;
	int matches;
	int wins;
	int winsWithFriends;
	int timesJumpmaster;
	int winsAsJumpmaster;
	int damage;
	int damageTaken;
	int kills;
	int teamworkKills;
	int timesRevivedAlly;
	int characterPickOrder;
	char characterName[32];
	char rankedPeriodName[32];
	int rankedScore;
};

struct CPlayerShared // !TODO: MOVE INTO SHARED!!!
{
	void* __vftable;
	int m_nPlayerCond;
	bool m_bLoadoutUnavailable;
	char gap_d[3];
	float m_nextWaterCheck;
	float m_flCondExpireTimeLeft[2];
	char gap_1c[4];
	void* m_pOuter;
	float m_flNextCritUpdate;
	float m_flTauntRemoveTime;
	CTakeDamageInfo m_damageInfo;
};

struct PushHistoryEntry
{
	float time;
	Vector3D pushed;
};

struct PredictableServerEvent
{
	int type;
	float deadlineTime;
	int fullSizeOfUnion[4];
};

struct CKnockBack
{
	void* __vftable;
	Vector3D velocity;
	float beginTime;
	float endTime;
};

struct SpeedChangeHistoryEntry
{
	float time;
	float maxSpeedScaled;
	float maxSpeedChange;
};


class CPlayer : public CBaseCombatCharacter
{
	friend class CPlayerMove;
public:
	void RunNullCommand(void);
	QAngle* EyeAngles(QAngle* pAngles);

	void SetTimeBase(float flTimeBase);
	void SetLastUCmdSimulationRemainderTime(int nRemainderTime);
	void SetTotalExtraClientCmdTimeAttempted(float flAttemptedTime);

	void ProcessUserCmds(CUserCmd* cmds, int numCmds, int totalCmds,
		int droppedPackets, bool paused);

	void PlayerRunCommand(CUserCmd* pUserCmd, IMoveHelper* pMover);
	void SetLastUserCommand(CUserCmd* pUserCmd);

	inline bool	IsConnected() const { return m_iConnected != PlayerDisconnected; }
	inline bool	IsDisconnecting() const { return m_iConnected == PlayerDisconnecting; }

	inline bool IsBot() const { return (GetFlags() & FL_FAKECLIENT) != 0; }

	inline NucleusID_t GetPlatformUserId() const { return m_platformUserId; };

	inline const char* GetNetName() const { return m_szNetname; }
	inline bool IsZooming() { return m_bZooming; }

private:
	char m_szNetname[256];
	bool m_zoomViewdriftDebounceEnabled;
	bool m_bZooming;
	char gap_5a62[2];
	float m_zoomToggleOnStartTime;
	float m_zoomBaseFrac;
	float m_zoomBaseTime;
	float m_zoomFullStartTime;
	int m_physicsSolidMask;
	int m_rightHandAttachment;
	int m_leftHandAttachment;
	int m_headAttachment;
	int m_chestAttachment;
	int m_StuckLast;
	char gap_5a8c[4];
	CPlayerLocalData m_Local;
	EHANDLE m_hColorCorrectionCtrl;
	char gap_5dbc[4];
	char m_hTriggerSoundscapeList[32];
	CPlayerState pl;
	int m_ammoPoolCapacity;
	bool m_hasBadReputation;
	char m_communityName[64];
	char m_communityClanTag[16];
	char m_factionName[16];
	char m_hardwareIcon[16];
	bool m_happyHourActive;
	char gap_5ee6[2];
	NucleusID_t m_platformUserId;
	char m_hardware;
	char gap_5ef1[7];
	i64 m_classModsActive;
	i64 m_classModsActiveOld;
	char m_classSettings[32];
	int m_classSettingsScriptRefCount;
	char m_modInventory[128];
	short m_consumableInventory[32];
	char gap_5fec[4];
	i64 m_passives[2];
	int m_bleedoutState;
	float m_damageComboLatestUpdateTime;
	int m_damageComboStartHealth;
	float m_healResources_nextTickTime[10];
	float m_healResources_healRatePerSecond[10];
	short m_healResources_remainingHeals[10];
	short m_healResources_totalHeals[10];
	short m_healResources_generation[10];
	short m_healResources_refId[10];
	EHANDLE m_healResources_healSource[10];
	int m_healResources_healthTarget;
	int m_communityId;
	int m_nButtons;
	int m_afButtonPressed;
	int m_afButtonReleased;
	int m_afButtonLast;
	int m_afButtonDisabled;
	int m_afButtonForced;
	float m_forwardMove;
	float m_sideMove;
	float m_prevForwardMove;
	float m_prevSideMove;
	bool m_bLagCompensation;
	bool m_bPredictWeapons;
	bool m_bPredictionEnabled;
	char gap_6107[1];
	int m_lagRecordValidationCounter;
	bool m_wantedToMatchmake;
	bool m_playerWeaponSwitchOnEquipIsDisabled;
	char gap_610e[2];
	EHANDLE m_skyCamera;
	Vector3D m_originalPlayerOrigin;
	EHANDLE m_playerVehicle;
	EHANDLE m_entitySyncingWithMe;
	int m_playerFlags;
	bool m_hasMic;
	bool m_inPartyChat;
	char gap_612e[2];
	float m_playerMoveSpeedScale;
	float m_cachedMoveScale;
	int m_gestureSequences[8];
	float m_gestureStartTimes[8];
	float m_gestureBlendInDuration[8];
	float m_gestureBlendOutDuration[8];
	float m_gestureFadeOutStartTime[8];
	float m_gestureFadeOutDuration[8];
	int m_gestureAutoKillBitfield;
	float m_flFallVelocityForLandAnims;
	bool m_bDropEnabled;
	bool m_bDuckEnabled;
	char gap_6202[2];
	int m_iRespawnFrames;
	int m_afPhysicsFlags;
	float m_flTimeLastTouchedWall;
	float m_timeJetpackHeightActivateCheckPassed;
	float m_flTimeLastTouchedGround;
	float m_flTimeLastJumped;
	float m_flTimeLastLanded;
	float m_flLastLandFromHeight;
	Vector3D m_upDirWhenLastTouchedGround;
	bool m_bHasJumpedSinceTouchedGround;
	bool m_bDoMultiJumpPenalty;
	bool m_bWasGrounded;
	char gap_6233[1];
	float m_usePressedTime;
	float m_lastUseTime;
	Vector3D m_lastFakeFloorPos;
	Vector3D m_accumDamageImpulseVel;
	float m_accumDamageImpulseTime;
	float m_damageImpulseNoDecelEndTime;
	EHANDLE m_hDmgEntity;
	float m_DmgTake;
	int m_bitsDamageType;
	int m_bitsHUDDamage;
	float m_xpRate;
	int m_gamepadIntegrityFailCount_RSPN;
	float m_lastKillTime;
	float m_flDeathTime;
	float m_flDeathAnimTime;
	int m_iObserverMode;
	int m_iObserverLastMode;
	EHANDLE m_hObserverTarget;
	Vector3D m_observerModeStaticPosition;
	Vector3D m_observerModeStaticAngles;
	float m_observerModeStaticFOVOverride;
	bool m_isValidChaseObserverTarget;
	bool m_observerModeToggleEnabled;
	char gap_62aa[2];
	int m_vphysicsCollisionState;
	bool m_bHasVPhysicsCollision;
	char gap_62b1[3];
	float m_fNextSuicideTime;
	int m_iSuicideCustomKillFlags;
	int m_preNoClipPhysicsFlags;
	CUserCmd* m_Commands;
	void* m_pPhysicsController;
	void* m_pShadowStand;
	void* m_pShadowCrouch;
	Vector3D m_oldOrigin;
	Vector3D m_vecSmoothedVelocity;
	bool m_bTouchedPhysObject;
	bool m_bPhysicsWasFrozen;
	bool m_fInitHUD;
	bool m_fGameHUDInitialized;
	int m_iUpdateTime;
	int m_iConnected;
	int m_iPlayerLocked;
	int m_gameStats[12];
	EHANDLE m_firstPersonProxy;
	EHANDLE m_predictedFirstPersonProxy;
	EHANDLE m_grappleHook;
	EHANDLE m_petTitan;
	int m_petTitanMode;
	int m_xp;
	int m_generation;
	int m_rank;
	int m_serverForceIncreasePlayerListGenerationParity;
	bool m_isPlayingRanked;
	char gap_635d[3];
	float m_skill_mu;
	EHANDLE m_hardpointEntity;
	float m_nextTitanRespawnAvailable;
	bool m_activeViewmodelModifiers[35];
	bool m_activeViewmodelModifiersChanged;
	EHANDLE m_hViewModels[3];
	CUserCmd m_LastCmd;
	CUserCmd* m_pCurrentCommand;
	float m_flStepSoundTime;
	EHANDLE m_hThirdPersonEnt;
	bool m_thirdPersonShoulderView;
	char gap_6589[7];
	ThirdPersonViewData m_thirdPerson;
	int m_duckState;
	int m_leanState;
	bool m_doingHalfDuck;
	bool m_canStand;
	char gap_65fa[2];
	Vector3D m_StandHullMin;
	Vector3D m_StandHullMax;
	Vector3D m_DuckHullMin;
	Vector3D m_DuckHullMax;
	Vector3D m_upDir;
	Vector3D m_upDirPredicted;
	Vector3D m_lastWallRunStartPos;
	float m_wallRunStartTime;
	float m_wallRunClearTime;
	int m_wallRunCount;
	bool m_wallRunWeak;
	bool m_shouldBeOneHanded;
	char gap_665e[2];
	float m_oneHandFraction;
	float m_animLastUpdateTime;
	float m_animAimPitch;
	float m_animAimYaw;
	float m_wallRunPushAwayTime;
	float m_wallrunFrictionScale;
	float m_groundFrictionScale;
	float m_wallrunRetryTime;
	Vector3D m_wallrunRetryPos;
	Vector3D m_wallrunRetryNormal;
	bool m_wallClimbSetUp;
	bool m_wallHanging;
	char gap_669a[2];
	float m_wallHangStartTime;
	float m_wallHangTime;
	int m_traversalType;
	int m_traversalState;
	Vector3D m_traversalBegin;
	Vector3D m_traversalMid;
	Vector3D m_traversalEnd;
	float m_traversalMidFrac;
	Vector3D m_traversalForwardDir;
	Vector3D m_traversalRefPos;
	float m_traversalProgress;
	float m_traversalStartTime;
	float m_traversalHandAppearTime;
	float m_traversalReleaseTime;
	float m_traversalBlendOutStartTime;
	Vector3D m_traversalBlendOutStartOffset;
	float m_traversalYawDelta;
	int m_traversalYawPoseParameter;
	float m_wallDangleJumpOffTime;
	bool m_wallDangleMayHangHere;
	bool m_wallDangleForceFallOff;
	bool m_wallDangleLastPushedForward;
	char gap_671b[1];
	int m_wallDangleDisableWeapon;
	float m_wallDangleClimbProgressFloor;
	float m_prevMoveYaw;
	float m_sprintTiltVel;
	int m_sprintTiltPoseParameter;
	int m_sprintFracPoseParameter;
	char gap_6734[4];
	GrappleData m_grapple;
	bool m_grappleActive;
	bool m_grappleNeedWindowCheck;
	char gap_67ba[2];
	EHANDLE m_grappleNextWindowHint;
	int m_landingType;
	bool m_slowMoEnabled;
	bool m_sliding;
	bool m_slideLongJumpAllowed;
	char gap_67c7[1];
	float m_lastSlideTime;
	float m_lastSlideBoost;
	bool m_ziplineAllowed;
	char gap_67d1[3];
	EHANDLE m_activeZipline;
	EHANDLE m_lastZipline;
	float m_lastZiplineDetachTime;
	bool m_ziplineValid3pWeaponLayerAnim;
	char gap_67e1[3];
	int m_ziplineState;
	PlayerZiplineData m_zipline;
	Vector3D m_ziplineViewOffsetPosition;
	Vector3D m_ziplineViewOffsetVelocity;
	EHANDLE m_ziplineGrenadeEntity;
	EHANDLE m_ziplineGrenadeBeginStationEntity;
	int m_ziplineGrenadeBeginStationAttachmentIndex;
	char gap_686c[4];
	Player_ViewOffsetEntityData m_viewOffsetEntity;
	Player_AnimViewEntityData m_animViewEntity;
	bool m_highSpeedViewmodelAnims;
	char gap_6911[3];
	int m_playAnimationType;
	bool m_detachGrappleOnPlayAnimationEnd;
	char gap_6919[3];
	int m_playAnimationNext[2];
	EHANDLE m_playAnimationEntityBlocker;
	bool m_playAnimationEntityBlockerCanTeleport;
	bool m_playAnimationEntityBlockerDucking;
	char gap_692a[2];
	float m_lastRunPlayerAnimationTime;
	int m_gravityGrenadeStatusEffect;
	float m_onSlopeTime;
	Vector3D m_lastWallNormal;
	bool m_dodgingInAir;
	bool m_dodging;
	char gap_6946[2];
	float m_lastDodgeTime;
	float m_airSpeed;
	float m_airAcceleration;
	bool m_iSpawnParity;
	bool m_boosting;
	bool m_activateBoost;
	bool m_repeatedBoost;
	float m_boostMeter;
	bool m_jetpack;
	bool m_activateJetpack;
	bool m_jetpackAfterburner;
	bool m_gliding;
	float m_glideMeter;
	float m_glideRechargeDelayAccumulator;
	bool m_hovering;
	bool m_isPerformingBoostAction;
	char gap_696a[2];
	float m_lastJumpHeight;
	char m_touchingSlipTriggers[72];
	Vector3D m_slipAirRestrictDirection;
	float m_slipAirRestrictTime;
	float m_lastTimeDamagedByOtherPlayer;
	float m_lastTimeDamagedByNPC;
	float m_lastTimeDidDamageToOtherPlayer;
	float m_lastTimeDidDamageToNPC;
	char m_predictedTriggers[72];
	EHANDLE m_lastJumpPadTouched;
	int m_launchCount;
	char m_lastBodySound1p[32];
	char m_lastBodySound3p[32];
	char m_lastFinishSound1p[32];
	char m_lastFinishSound3p[32];
	char m_primedSound1p[32];
	char m_primedSound3p[32];
	int m_replayImportantSounds_networkTableSoundID[4];
	float m_replayImportantSounds_beginTime[4];
	CurrentData_Player m_currentFramePlayer;
	CurrentData_LocalPlayer m_currentFrameLocalPlayer;
	int m_nImpulse;
	int m_nNumCrateHudHints;
	bool m_needStuckCheck;
	char gap_6bd9[3];
	float m_totalFrameTime;
	float m_joinFrameTime;
	int m_lastUCmdSimulationTicks;
	int m_lastUCmdSimulationRemainderTime; // Originally float???
	float m_totalExtraClientCmdTimeAttempted;
	bool m_atLeastOneCommandRunThisServerFrame;
	bool m_bGamePaused;
	bool m_bPlayerUnderwater;
	bool m_wasPhaseShiftedForTriggers;
	EHANDLE m_hPlayerViewEntity;
	bool m_bShouldDrawPlayerWhileUsingViewEntity;
	char gap_6bf9[3];
	EHANDLE m_hConstraintEntity;
	Vector3D m_vecConstraintCenter;
	float m_flConstraintRadius;
	float m_flConstraintWidth;
	float m_flConstraintSpeedFactor;
	bool m_bConstraintPastRadius;
	char gap_6c19[7];
	CBitVec<64> m_twitchRewardBits;
	bool m_twitchRewardsUnfulfilled;
	char gap_6c29[3];
	float m_lastActiveTime;
	float m_flLaggedMovementValue;
	float m_lastMoveInputTime;
	EHANDLE m_ignoreEntityForMovementUntilNotTouching;
	Vector3D m_vNewVPhysicsPosition;
	Vector3D m_vNewVPhysicsVelocity;
	Vector3D m_vNewVPhysicsWishVel;
	Vector3D m_vecPreviouslyPredictedOrigin;
	Vector3D m_prevAbsOrigin;
	Vector3D m_preMoveThinkAbsOrigin;
	int m_nBodyPitchPoseParam;
	char m_szNetworkIDString[64];
	void* m_squad;
	string_t m_SquadName;
	char m_gameMovementUtil[56];
	float m_flTimeAllSuitDevicesOff;
	bool m_bIsStickySprinting;
	char gap_6d15[3];
	float m_fStickySprintMinTime;
	bool m_bPlayedSprintStartEffects;
	char gap_6d1d[3];
	int m_autoSprintForced;
	bool m_fIsSprinting;
	bool m_fIsWalking;
	char gap_6d26[2];
	float m_sprintStartedTime;
	float m_sprintStartedFrac;
	float m_sprintEndedTime;
	float m_sprintEndedFrac;
	float m_stickySprintStartTime;
	bool m_bSinglePlayerGameEnding;
	char gap_6d3d[3];
	int m_ubEFNoInterpParity;
	bool m_viewConeActive;
	bool m_viewConeParented;
	char gap_6d46[2];
	int m_viewConeParity;
	int m_lastViewConeParityTick;
	float m_viewConeLerpTime;
	bool m_viewConeSpecificEnabled;
	char gap_6d55[3];
	Vector3D m_viewConeSpecific;
	Vector3D m_viewConeRelativeAngleMin;
	Vector3D m_viewConeRelativeAngleMax;
	EHANDLE m_hReservedSpawnPoint;
	EHANDLE m_hLastSpawnPoint;
	MatchMetrics m_matchMetrics;
	bool m_autoKickDisabled;
	char gap_6e11[3];
	EHANDLE m_stuckCharacter;
	char m_title[32];
	bool sentHUDScriptChecksum;
	bool m_bIsFullyConnected;
	char gap_6e3a[2];
	CTakeDamageInfo m_lastDeathInfo;
	char gap_6ebc[4];
	PlayerMelee_PlayerData m_melee;
	EHANDLE m_lungeTargetEntity;
	bool m_isLungingToPosition;
	char gap_6efd[3];
	Vector3D m_lungeTargetPosition;
	Vector3D m_lungeStartPositionOffset;
	Vector3D m_lungeStartPositionOffset_notLagCompensated;
	Vector3D m_lungeEndPositionOffset;
	float m_lungeStartTime;
	float m_lungeEndTime;
	bool m_lungeCanFly;
	bool m_lungeLockPitch;
	char gap_6f3a[2];
	float m_lungeStartPitch;
	float m_lungeSmoothTime;
	float m_lungeMaxTime;
	float m_lungeMaxEndSpeed;
	bool m_useCredit;
	char gap_6f4d[3];
	__int64 m_smartAmmoNextTarget;
	__int64 m_smartAmmoPrevTarget;
	float m_smartAmmoHighestLocksOnMeFractionValues[4];
	EHANDLE m_smartAmmoHighestLocksOnMeEntities[4];
	float m_smartAmmoPreviousHighestLockOnMeFractionValue;
	float m_smartAmmoPendingHighestLocksOnMeFractionValues[4];
	char gap_6f94[4];
	void* m_smartAmmoPendingHighestLocksOnMeEntities[4];
	bool m_smartAmmoRemoveFromTargetList;
	char gap_6fb9[3];
	int m_delayedFlinchEvents;
	i64 m_delayedFlinchEventCount;
	char m_extraWeaponModNames[512];
	char m_extraWeaponModNamesArray[64];
	i64 m_extraWeaponModNameCount;
	char m_pPlayerAISquad[8];
	float m_flAreaCaptureScoreAccumulator;
	float m_flCapPointScoreRate;
	float m_flConnectionTime;
	float m_fullyConnectedTime;
	float m_connectedForDurationCallback_duration;
	float m_flLastForcedChangeTeamTime;
	int m_iBalanceScore;
	char gap_7234[4];
	void* m_PlayerAnimState;
	Vector3D m_vWorldSpaceCenterHolder;
	Vector3D m_vPrevGroundNormal;
	int m_threadedPostProcessJob;
	char gap_725c[4];
	CPlayerShared m_Shared;
	StatusEffectTimedData m_statusEffectsTimedPlayerNV[10];
	StatusEffectEndlessData m_statusEffectsEndlessPlayerNV[10];
	int m_pilotClassIndex;
	char m_pilotClassActivityModifier[2];
	bool m_pilotClassActivityModifierInitialized;
	char gap_74a7[1];
	int m_latestCommandRun;
	int m_latestCommandQueued;
	float m_lastSimulateTime;
	char gap_74b4[4];
	char m_nearbyPushers[480];
	int m_nearbyPusherCount;
	PushHistoryEntry m_pushHistory[16];
	int m_pushHistoryEntryIndex;
	float m_baseVelocityLastServerTime;
	Vector3D m_pushedThisFrame;
	Vector3D m_pushedThisSnapshotAccum;
	EHANDLE m_pusher;
	float m_pusherTouchTime;
	float m_pusherUntouchTime;
	Vector3D m_originRelativeToPusher;
	EHANDLE m_predictingEnts[4];
	int m_predictingEntCount;
	float m_lastCommandCountWarnTime;
	Vector3D m_pushAwayFromTopAcceleration;
	float m_trackedState[52];
	int m_prevTrackedState;
	Vector3D m_prevTrackedStatePos;
	char m_recordingAnim[8];
	char m_animRecordFile[8];
	char m_animRecordButtons[4];
	float m_minimapTargetZoomScale;
	float m_minimapTargetLerpTime;
	bool m_sendMovementCallbacks;
	bool m_sendInputCallbacks;
	char gap_78f6[2];
	PredictableServerEvent m_predictableServerEvents[16];
	int m_predictableServerEventCount;
	int m_predictableServerEventAcked;
	EHANDLE m_playerScriptNetDataGlobal;
	EHANDLE m_playerScriptNetDataExclusive;
	int m_armorType;
	int m_helmetType;
	bool m_controllerModeActive;
	bool m_freefallNoCollisionFoundAroundPlayer;
	char gap_7a92[2];
	Vector3D m_freefallCapsuleStart;
	Vector3D m_freefallCapsuleEnd;
	Vector3D m_freefallCapsuleHalfHeight;
	float m_freefallCapsuleRadius;
	Vector3D m_freefallCapsuleCorners[4];
	char gap_7aec[4];
	Vector4D m_freefallCapsuleSweepPlane;
	Vector4D m_freefallCapsuleEdgePlanes[4];
	float m_freefallJobSearchRange;
	float m_skydiveForwardPoseValueVelocity;
	float m_skydiveForwardPoseValueTarget;
	float m_skydiveForwardPoseValueCurrent;
	float m_skydiveSidePoseValueVelocity;
	float m_skydiveSidePoseValueTarget;
	float m_skydiveSidePoseValueCurrent;
	float m_skydiveYawVelocity;
	int m_freefallState;
	float m_freefallStartTime;
	float m_freefallEndTime;
	float m_freefallAnticipateStartTime;
	float m_freefallAnticipateEndTime;
	float m_freefallDistanceToLand;
	float m_skydiveDiveAngle;
	bool m_skydiveIsDiving;
	char gap_7b7d[3];
	float m_skydiveSpeed;
	float m_skydiveStrafeAngle;
	bool m_skydiveFreelookEnabled;
	char gap_7b89[3];
	Vector3D m_skydiveFreelookLockedAngle;
	float m_skydivePlayerPitch;
	float m_skydivePlayerYaw;
	bool m_skydiveFollowing;
	char gap_7ba1[3];
	Vector3D m_skydiveUnfollowVelocity;
	bool m_skydiveIsNearLeviathan;
	char gap_7bb1[3];
	Vector3D m_skydiveLeviathanHitPosition;
	Vector3D m_skydiveLeviathanHitNormal;
	Vector3D m_skydiveSlipVelocity;
	bool m_forceWeaponReload;
	char gap_7bd9[7];
	CKnockBack m_playerKnockBacks[4];
	SpeedChangeHistoryEntry m_speedChangeHistory[32];
	int m_speedChangeHistoryIndex;
	bool m_hasValidTraceToKnockBackEyePosition;
	char m_bodyModelOverride[256];
	char gap_7ee5[3];
	int m_armsModelIndex;
};
static_assert(sizeof(CPlayer) == 0x7EF0);

inline QAngle*(*CPlayer__EyeAngles)(CPlayer* pPlayer, QAngle* pAngles);
inline void(*CPlayer__PlayerRunCommand)(CPlayer* pPlayer, CUserCmd* pUserCmd, IMoveHelper* pMover);
inline bool(*CPlayer__PhysicsSimulate)(CPlayer* pPlayer, int numPerIteration, bool adjustTimeBase);
inline void(*CPlayer__ApplyViewPunch)(CPlayer* pPlayer, const CTakeDamageInfo* inputInfo);

///////////////////////////////////////////////////////////////////////////////
class VPlayer : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CPlayer::EyeAngles", CPlayer__EyeAngles);
		LogFunAdr("CPlayer::PlayerRunCommand", CPlayer__PlayerRunCommand);
		LogFunAdr("CPlayer::PhysicsSimulate", CPlayer__PhysicsSimulate);
		LogFunAdr("CPlayer::ApplyViewPunch", CPlayer__ApplyViewPunch);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 F2 0F 10 05 ?? ?? ?? ??").GetPtr(CPlayer__EyeAngles);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 8B 03 49 81 C6 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(CPlayer__PlayerRunCommand);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ?? 84 C0 74 06").FollowNearCallSelf().GetPtr(CPlayer__PhysicsSimulate);
		g_GameDll.FindPatternSIMD("4C 8B DC 49 89 5B ?? 49 89 6B ?? 49 89 7B ?? 41 54 41 56 41 57 48 81 EC").GetPtr(CPlayer__ApplyViewPunch);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // PLAYER_H
