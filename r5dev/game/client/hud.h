//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CHud handles the message, calculation, and drawing the HUD
//
// $NoKeywords: $
//=============================================================================//
#ifndef HUD_H
#define HUD_H

// basic rectangle struct used for drawing
typedef struct wrect_s
{
	int	left;
	int right;
	int top;
	int bottom;
} wrect_t;

#endif // HUD_H