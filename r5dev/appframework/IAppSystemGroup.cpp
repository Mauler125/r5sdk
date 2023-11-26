//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "appframework/IAppSystemGroup.h"

//-----------------------------------------------------------------------------
// Purpose: Initialize plugin system
//-----------------------------------------------------------------------------
void CAppSystemGroup::StaticDestroy(CAppSystemGroup* pModAppSystemGroup)
{
	CAppSystemGroup_Destroy(pModAppSystemGroup);
}

//-----------------------------------------------------------------------------
// Returns the stage at which the app system group ran into an error
//-----------------------------------------------------------------------------
CAppSystemGroup::AppSystemGroupStage_t CAppSystemGroup::GetCurrentStage() const
{
	return m_nCurrentStage;
}

//-----------------------------------------------------------------------------
// Methods to find various global singleton systems 
//-----------------------------------------------------------------------------
void* CAppSystemGroup::FindSystem(const char* pSystemName)
{
	unsigned short i = m_SystemDict.Find(pSystemName);
	if (i != m_SystemDict.InvalidIndex())
		return m_Systems[m_SystemDict[i]];

	// If it's not an interface we know about, it could be an older
	// version of an interface, or maybe something implemented by
	// one of the instantiated interfaces...

	// QUESTION: What order should we iterate this in?
	// It controls who wins if multiple ones implement the same interface
	for (i = 0; i < m_Systems.Count(); ++i)
	{
		void* pInterface = m_Systems[i]->QueryInterface(pSystemName);
		if (pInterface)
			return pInterface;
	}

	int nExternalCount = m_NonAppSystemFactories.Count();
	for (i = 0; i < nExternalCount; ++i)
	{
		void* pInterface = m_NonAppSystemFactories[i](pSystemName, NULL);
		if (pInterface)
			return pInterface;
	}

	if (m_pParentAppSystem)
	{
		void* pInterface = m_pParentAppSystem->FindSystem(pSystemName);
		if (pInterface)
			return pInterface;
	}

	// No dice..
	return NULL;
}

void VAppSystemGroup::Detour(const bool bAttach) const
{
	DetourSetup(&CAppSystemGroup_Destroy, &CAppSystemGroup::StaticDestroy, bAttach);
}
