//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#ifndef ENTITYLIST_H
#define ENTITYLIST_H
#include "game/shared/entitylist_base.h"

IHandleEntity* LookupEntityByIndex(int iEntity);
extern CEntInfo** g_pEntityList;

#endif // ENTITYLIST_H
