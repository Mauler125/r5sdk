//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
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

class CEntInfoList
{
public:
	CEntInfoList();

	const CEntInfo	*Head() const { return m_pHead; }
	const CEntInfo	*Tail() const { return m_pTail; }
	CEntInfo		*Head() { return m_pHead; }
	CEntInfo		*Tail() { return m_pTail; }
	//void			AddToHead( CEntInfo *pElement ) { LinkAfter( NULL, pElement ); }
	//void			AddToTail( CEntInfo *pElement ) { LinkBefore( NULL, pElement ); }

	//void LinkBefore( CEntInfo *pBefore, CEntInfo *pElement );
	//void LinkAfter( CEntInfo *pBefore, CEntInfo *pElement );
	//void Unlink( CEntInfo *pElement );
	//bool IsInList( CEntInfo *pElement );

private:
	CEntInfo		*m_pHead;
	CEntInfo		*m_pTail;
};

#endif // ENTITYLIST_BASE_H
