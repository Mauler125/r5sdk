//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the client-side representation of CBaseAnimating.
//
// $NoKeywords: $
//===========================================================================//
#ifndef C_BASEANIMATING_H
#define C_BASEANIMATING_H

#include "c_baseentity.h"
#include "game/shared/animation.h"

struct C_SequenceTransitionerLayer
{
	void* _vftable;
	bool bunk0;
	CBaseEntity* m_pOuter;
	bool m_sequenceTransitionerLayerActive;
	char gap_19[3];
	float m_sequenceTransitionerLayerStartCycle;
	int m_sequenceTransitionerLayerSequence;
	float m_weight;
	float m_sequenceTransitionerLayerPlaybackRate;
	float m_sequenceTransitionerLayerStartTime;
	float m_sequenceTransitionerLayerFadeOutDuration;
};


struct C_SequenceTransitioner
{
	void* _vftable;
	char _pad_unk[16];
	C_SequenceTransitionerLayer m_sequenceTransitionerLayers[7];
	char gap_50[336];
	int m_sequenceTransitionerLayerCount;
};

class C_BaseAnimating : public C_BaseEntity
{
	char gap_a00[200];
	PredictedAnimEventData m_predictedAnimEventData;
	char gap_b28[152];
	C_SequenceTransitioner m_SequenceTransitioner;
	char gap_bc0[648];
	int m_nSkin;
	__int16 m_skinMod;
	char gap_e4e[2];
	int m_nBody;
	char gap_e54[8];
	int m_nResetEventsParity;
	char gap_e60[160];
	bool m_bSequenceFinished;
	char gap_f01[7];
	bool m_bSequenceLooped;
	bool m_bSequenceLoops;
	char gap_f0a[2];
	float m_flModelScale;
	char gap_f10[8];
	float m_currentFrameBaseAnimating__animStartTime;
	float m_currentFrameBaseAnimating__animStartCycle;
	float m_currentFrameBaseAnimating__animPlaybackRate;
	char gap_f24[4];
	int m_currentFrameBaseAnimating__animModelIndex;
	int m_currentFrameBaseAnimating__animSequence;
	int m_currentFrameBaseAnimating__animSequenceParity;
	float m_currentFrameBaseAnimating__m_flPoseParameters[24];
	char pad[112];
};

static_assert(sizeof(C_BaseAnimating) == 0x1300);

#endif // C_BASEANIMATING_H
