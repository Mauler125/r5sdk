//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "core/stdafx.h"
#include "movehelper_client.h"

//-----------------------------------------------------------------------------
// Purpose: Gets the client movehelper
//-----------------------------------------------------------------------------
IMoveHelper* MoveHelperClient()
{
	return s_MoveHelperClient;
}

CMoveHelperClient* s_MoveHelperClient = nullptr;