//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "game/client/enginesprite.h"

//-----------------------------------------------------------------------------
// Is the sprite an AVI?
//-----------------------------------------------------------------------------
bool CEngineSprite::IsAVI(void) const
{
	return (m_hAVIMaterial != AVIMATERIAL_INVALID);
}

//-----------------------------------------------------------------------------
// Is the sprite an BIK?
//-----------------------------------------------------------------------------
bool CEngineSprite::IsBIK(void) const
{
	return (m_hBIKMaterial != BIKMATERIAL_INVALID);
}
