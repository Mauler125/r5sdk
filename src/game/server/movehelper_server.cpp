//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "core/stdafx.h"
#include "movehelper_server.h"

//-----------------------------------------------------------------------------
// Purpose: Gets the server movehelper
//-----------------------------------------------------------------------------
IMoveHelper* MoveHelperServer()
{
	return s_MoveHelperServer;
}

CMoveHelperServer* s_MoveHelperServer = nullptr;