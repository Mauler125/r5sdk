//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHARED_CLASSNAMES_H
#define SHARED_CLASSNAMES_H
#ifdef _WIN32
#pragma once
#endif

// Hacky macros to allow shared code to work without even worse macro-izing
#if defined( CLIENT_DLL )
 // Uncomment if required for client.
#define CBaseEntity				C_BaseEntity
#define CBaseCombatCharacter	C_BaseCombatCharacter
#define CBaseAnimating			C_BaseAnimating
#define CPlayer					C_Player

#endif


#endif // SHARED_CLASSNAMES_H
