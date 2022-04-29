//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#ifndef IBIK_H
#define IBIK_H

//-----------------------------------------------------------------------------
// Handle to an BINK
//-----------------------------------------------------------------------------
typedef unsigned short BIKHandle_t;
enum
{
	BIKHANDLE_INVALID = (BIKHandle_t)~0
};


//-----------------------------------------------------------------------------
// Handle to an BINK material
//-----------------------------------------------------------------------------
typedef unsigned short BIKMaterial_t;
enum
{
	BIKMATERIAL_INVALID = (BIKMaterial_t)~0
};

#endif // IBIK_H