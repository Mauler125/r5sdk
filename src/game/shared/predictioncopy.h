//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTIONCOPY_H
#define PREDICTIONCOPY_H
#ifdef _WIN32
#pragma once
#endif

#include "ehandle.h"

#if defined( CLIENT_DLL )
class C_BaseEntity;
typedef CHandle<C_BaseEntity> EHANDLE;
#else
class CBaseEntity;
typedef CHandle<CBaseEntity> EHANDLE;
#endif

#endif // PREDICTIONCOPY_H
