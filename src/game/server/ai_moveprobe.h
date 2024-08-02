//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: AI move probe
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_MOVEPROBE_H
#define AI_MOVEPROBE_H

#if defined( _WIN32 )
#pragma once
#endif

#include "ai_component.h"
#include "game/shared/predictioncopy.h"

class CAI_MoveProbe : CAI_Component
{
	bool m_bIgnoreTransientEntities;
	char gap_11[3];
	Vector3D m_vecFloorPoint;
	float m_floorPointTime;
	bool m_floorPointStandable;
	char gap_25[3];
	EHANDLE m_hLastProbeBlockingEnt;
};
static_assert(sizeof(CAI_MoveProbe) == 0x30);

#endif // AI_MOVEPROBE_H
