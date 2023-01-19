//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYLIST_BASE_H
#define ENTITYLIST_BASE_H
#ifdef _WIN32
#pragma once
#endif
#include "public/ihandleentity.h"

class CEntInfo
{
public:
	IHandleEntity* m_pEntity;
	int			   m_SerialNumber;
	CEntInfo* m_pPrev;
	CEntInfo* m_pNext;
#ifdef GAME_DLL	
	string_t		m_iName;
	string_t		m_iClassName;
#endif

	void			ClearLinks();
};


#endif // ENTITYLIST_BASE_H