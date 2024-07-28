//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//-----------------------------------------------------------------------------
#include "baseentity.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------
// Returns the sound origin
// todo: requires implementing CBaseEntity::GetAbsOrigin.
//---------------------------------------------------------
/*const Vector3D& CSound::GetSoundOrigin(void)
{
	if ((m_iType & SOUND_CONTEXT_FOLLOW_OWNER) != 0)
	{
		if (m_hOwner.Get() != NULL)
			return m_hOwner->GetAbsOrigin();
	}
	return m_vecOrigin;
}*/