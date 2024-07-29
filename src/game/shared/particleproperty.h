//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PARTICLEPROPERTY_H
#define PARTICLEPROPERTY_H

class CParticleProperty
{
private:
	void* _vftable;
	CBaseEntity* m_pOuter;
	CUtlVector<int> m_ParticleEffects; // TODO: reverse ParticleControlPoint_t
	int m_iDormancyChangedAtFrame;

	friend class CBaseEntity;
};

#endif // PARTICLEPROPERTY_H
