//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Squad classes
//
//=============================================================================//

#ifndef AI_MOVESHOOT_H
#define AI_MOVESHOOT_H

#include "ai_component.h"

struct CAI_MoveAndShootOverlay : public CAI_Component
{
	int m_forcedMovement;
	float m_forcedMovementExpireTime;
	bool m_isMovingAndShooting;
	bool m_disableMoveAndShoot;
};
static_assert(sizeof(CAI_MoveAndShootOverlay) == 0x20);

#endif // AI_MOVESHOOT_H