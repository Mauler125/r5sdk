//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. =====//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "modelinfo.h"

#ifndef CLIENT_DLL
CModelInfoServer* g_pModelInfoServer = nullptr;
#endif // CLIENT_DLL

#ifndef DEDICATED
CModelInfoClient* g_pModelInfoClient = nullptr;
#endif // DEDICATED
