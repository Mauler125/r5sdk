//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "IAppSystemGroup.h"

//-----------------------------------------------------------------------------
// Purpose: Initialize plugin system
//-----------------------------------------------------------------------------
void CAppSystemGroup::Destroy(CAppSystemGroup* pModAppSystemGroup)
{
	CAppSystemGroup_Destroy(pModAppSystemGroup);
}

void VAppSystemGroup::Attach(void) const
{
	DetourAttach(&CAppSystemGroup_Destroy, &CAppSystemGroup::Destroy);
}
void VAppSystemGroup::Detach(void) const
{
	DetourDetach(&CAppSystemGroup_Destroy, &CAppSystemGroup::Destroy);
}