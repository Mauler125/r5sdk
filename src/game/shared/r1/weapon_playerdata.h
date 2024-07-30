#ifndef SHARED_WEAPON_PLAYERDATA_H
#define SHARED_WEAPON_PLAYERDATA_H

class WeaponPlayerData
{
	void* __vftable;
	float m_moveSpread;
	float m_spreadStartTime;
	float m_spreadStartFracHip;
	float m_spreadStartFracADS;
	float m_kickSpreadHipfire;
	float m_kickSpreadADS;
	float m_kickTime;
	float m_kickScaleBasePitch;
	float m_kickScaleBaseYaw;
	float m_kickPatternScaleBase;
	float m_kickSpringHeatBaseTime;
	float m_kickSpringHeatBaseValue;
	float m_semiAutoTriggerHoldTime;
	bool m_semiAutoTriggerDown;
	bool m_pendingTriggerPull;
	bool m_semiAutoNeedsRechamber;
	bool m_pendingReloadAttempt;
	bool m_offhandHybridNormalMode;
	bool m_pendingoffhandHybridToss;
	bool m_fastHolster;
	bool m_didFirstDeploy;
	bool m_shouldCatch;
	bool m_clipModelIsHidden;
	bool m_segmentedReloadEndSeqRequired;
	bool m_reloadStartedEmpty;
	bool m_segmentedAnimStartedOneHanded;
	bool m_segmentedReloadCanRestartLoop;
	bool m_segmentedReloadLoopFireLocked;
	char m_realtimeModCmds[8];
	char m_realtimeModCmdHead;
	char m_realtimeModCmdCount;
	char gap_55[3];
	int m_customActivityAttachedModelIndex;
	int m_customActivityAttachedModelAttachmentIndex;
	float m_fireRateLerp_startTime;
	float m_fireRateLerp_startFraction;
	float m_fireRateLerp_stopTime;
	float m_fireRateLerp_stopFraction;
	int m_chargeAnimIndex;
	int m_chargeAnimIndexOld;
	EHANDLE m_proScreen_owner;
	int m_proScreen_int0;
	int m_proScreen_int1;
	int m_proScreen_int2;
	float m_proScreen_float0;
	float m_proScreen_float1;
	float m_proScreen_float2;
	int m_reloadMilestone;
	int m_rechamberMilestone;
	int m_cooldownMilestone;
	float m_fullReloadStartTime;
	float m_scriptTime0;
	int m_scriptFlags0;
	int m_scriptInt0;
	float m_curZoomFOV;
	float m_targetZoomFOV;
	float m_zoomFOVLerpTime;
	float m_zoomFOVLerpEndTime;
	float m_latestDryfireTime;
	float m_requestedAttackEndTime;
	int m_currentAltFireAnimIndex;
	int m_legendaryModelIndex;
	int m_charmModelIndex;
	int m_charmAttachment;
	int m_charmScriptIndex;
};

// Client's class is identical.
typedef WeaponPlayerData WeaponPlayerData_Client;

#endif // SHARED_WEAPON_PLAYERDATA_H
