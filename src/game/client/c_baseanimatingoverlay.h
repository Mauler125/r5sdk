//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the client-side representation of CBaseAnimatingOverlay.
//
// $NoKeywords: $
//===========================================================================//
#ifndef C_BASEANIMATINGOVERLAY_H
#define C_BASEANIMATINGOVERLAY_H

#include "c_baseanimating.h"
#include "animationlayer.h"

class C_BaseAnimatingOverlay : public C_BaseAnimating
{
	int unknownDword;
	C_AnimationLayer m_AnimOverlay[9];
	char gap_1308[216];
	int m_AnimOverlayCount;
	char gap_13e4_this_may_be_incorrect[44];
	bool m_currentFrameAnimatingOverlay__animOverlayIsActive[9];
	char gap_142d[3];
	float m_currentFrameAnimatingOverlay__animOverlayStartTime[9];
	float m_currentFrameAnimatingOverlay__animOverlayStartCycle[9];
	float m_currentFrameAnimatingOverlay__animOverlayPlaybackRate[9];
	int m_currentFrameAnimatingOverlay__animOverlayModelIndex[9];
	int m_currentFrameAnimatingOverlay__animOverlaySequence[9];
	float m_currentFrameAnimatingOverlay__animOverlayWeight[9];
	int m_currentFrameAnimatingOverlay__animOverlayOrder[9];
	float m_currentFrameAnimatingOverlay__animOverlayAnimTime[9];
	float m_currentFrameAnimatingOverlay__animOverlayFadeInDuration[9];
	float m_currentFrameAnimatingOverlay__animOverlayFadeOutDuration[9];
	float m_currentFrameAnimatingOverlay__animOverlayCycle[9];
};

static_assert(sizeof(C_BaseAnimatingOverlay) == 0x1680);

#endif // C_BASEANIMATINGOVERLAY_H
