//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//          groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================//
#include "baseentity.h"
#include "ai_interest.h"

bool CAI_InterestTarget_t::IsThis(CBaseEntity* pThis)
{
	return (pThis == m_hTarget);
};

// TODO: requires rebuild of CBaseEntity::EyePosition
//const Vector3D& CAI_InterestTarget_t::GetPosition(void)
//{
//	if (m_eType == LOOKAT_ENTITY && m_hTarget != NULL)
//	{
//		m_vecPosition = m_hTarget->EyePosition();
//	}
//	return m_vecPosition;
//};

bool CAI_InterestTarget_t::IsActive(void)
{
	if (m_flEndTime < (*g_pGlobals)->m_flCurTime) return false;
	if (m_eType == LOOKAT_ENTITY && m_hTarget == NULL) return false;
	return true;
};

float CAI_InterestTarget_t::Interest(void)
{
	float t = ((*g_pGlobals)->m_flCurTime - m_flStartTime) / (m_flEndTime - m_flStartTime);

	if (t < 0.0f || t > 1.0f)
		return 0.0f;

	if (m_rampDuration && t < 1 - m_rampDuration)
	{
		//t = t / m_flRamp;
		t = 1.0f - ExponentialDecay(0.2f, m_rampDuration, t);
		//t = 1.0 - ExponentialDecay( 0.01, 1 - m_flRamp, t );
	}
	else if (t > 1.0f - m_rampDuration)
	{
		t = (1.0f - t) / m_rampDuration;
		t = 3.0f * t * t - 2.0f * t * t * t;
	}
	else
	{
		t = 1.0f;
	}
	// ramp
	t *= m_flInterest;

	return t;
}

void CAI_InterestTarget::Add(CBaseEntity* pTarget, float flImportance, float flDuration, float flRamp)
{
	int i;

	for (i = 0; i < Count(); i++)
	{
		CAI_InterestTarget_t& target = Element(i);

		if (target.m_hTarget == pTarget && target.m_rampDuration == 0)
		{
			if (target.m_flStartTime == (*g_pGlobals)->m_flCurTime)
			{
				flImportance = MAX(flImportance, target.m_flInterest);
			}
			Remove(i);
			break;
		}
	}

	Add(CAI_InterestTarget_t::LOOKAT_ENTITY, pTarget, Vector3D(0, 0, 0), flImportance, flDuration, flRamp);
}

void CAI_InterestTarget::Add(const Vector3D& vecPosition, float flImportance, float flDuration, float flRamp)
{
	int i;

	for (i = 0; i < Count(); i++)
	{
		CAI_InterestTarget_t& target = Element(i);

		if (target.m_vecPosition == vecPosition)
		{
			Remove(i);
			break;
		}
	}

	Add(CAI_InterestTarget_t::LOOKAT_POSITION, NULL, vecPosition, flImportance, flDuration, flRamp);
}

void CAI_InterestTarget::Add(CBaseEntity* pTarget, const Vector3D& vecPosition, float flImportance, float flDuration, float flRamp)
{
	int i;

	for (i = 0; i < Count(); i++)
	{
		CAI_InterestTarget_t& target = Element(i);

		if (target.m_hTarget == pTarget)
		{
			if (target.m_flStartTime == (*g_pGlobals)->m_flCurTime)
			{
				flImportance = MAX(flImportance, target.m_flInterest);
			}
			Remove(i);
			break;
		}
	}

	Add(CAI_InterestTarget_t::LOOKAT_BOTH, pTarget, vecPosition, flImportance, flDuration, flRamp);
}

void CAI_InterestTarget::Add(CAI_InterestTarget_t::CAI_InterestTarget_e type, CBaseEntity* pTarget, const Vector3D& vecPosition, float flImportance, float flDuration, float flRamp)
{
	int i = AddToTail();
	CAI_InterestTarget_t& target = Element(i);

	target.m_eType = type;
	target.m_hTarget = pTarget;
	target.m_vecPosition = vecPosition;
	target.m_flInterest = flImportance;
	target.m_flStartTime = (*g_pGlobals)->m_flCurTime;
	target.m_flEndTime = (*g_pGlobals)->m_flCurTime + flDuration;
	target.m_rampDuration = flRamp / flDuration;
}