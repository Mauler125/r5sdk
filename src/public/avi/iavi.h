//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#ifndef IAVI_H
#define IAVI_H

//-----------------------------------------------------------------------------
// Handle to an AVI
//-----------------------------------------------------------------------------
typedef unsigned short AVIHandle_t;
enum
{
	AVIHANDLE_INVALID = (AVIHandle_t)~0
};


//-----------------------------------------------------------------------------
// Handle to an AVI material
//-----------------------------------------------------------------------------
typedef unsigned short AVIMaterial_t;
enum
{
	AVIMATERIAL_INVALID = (AVIMaterial_t)~0
};

#endif // IAVI_H