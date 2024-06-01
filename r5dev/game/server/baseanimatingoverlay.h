//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef BASE_ANIMATING_OVERLAY_H
#define BASE_ANIMATING_OVERLAY_H
#ifdef _WIN32
#pragma once
#endif
#include "baseanimating.h"

class CBaseAnimatingOverlay : public CBaseAnimating
{
	void* __vftable;
	int m_maxOverlays;
	char gap_11f4[4];
	CAnimationLayer m_AnimOverlay;
	char gap_1228[384];
	int m_AnimOverlayCount;
	char m_overlayEventParity[9];
	bool m_animOverlayIsActive[9];
	char gap_13be[2];
	int m_animOverlayModelIndex[9];
	int m_animOverlaySequence[9];
	float m_animOverlayCycle[9];
	float m_animOverlayStartTime[9];
	float m_animOverlayStartCycle[9];
	float m_animOverlayPlaybackRate[9];
	float m_animOverlayWeight[9];
	int m_animOverlayOrder[9];
	float m_animOverlayAnimTime[9];
	float m_animOverlayFadeInDuration[9];
	float m_animOverlayFadeOutDuration[9];
	bool m_localAnimOverlayIsActive[4];
	int m_localAnimOverlayModelIndex[4];
	int m_localAnimOverlaySequence[4];
	float m_localAnimOverlayStartTime[4];
	float m_localAnimOverlayWeight[4];
	float m_localAnimOverlayFadeInDuration[4];
	float m_localAnimOverlayFadeOutDuration[4];
};

#endif // BASE_ANIMATING_OVERLAY_H
