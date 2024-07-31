//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "entitylist.h"

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game. All iteration through
//          entities is done through this object.
//-----------------------------------------------------------------------------
CGlobalEntityList* g_serverEntityList = nullptr;