//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef CONST_H
#define CONST_H

// How many bits to use to encode an edict.
#define	MAX_EDICT_BITS				14			// # of bits needed to represent max edicts
// Max # of edicts in a level
#define	MAX_EDICTS					(1<<MAX_EDICT_BITS)
// Used for networking ehandles.
#define NUM_ENT_ENTRY_BITS		(MAX_EDICT_BITS + 2)
#define NUM_ENT_ENTRIES			(1 << NUM_ENT_ENTRY_BITS) // Value is correct for r5.
#define INVALID_EHANDLE_INDEX	0xFFFFFFFF

#define NUM_SERIAL_NUM_BITS		16 // (32 - NUM_ENT_ENTRY_BITS)
#define NUM_SERIAL_NUM_SHIFT_BITS (32 - NUM_SERIAL_NUM_BITS)
#define ENT_ENTRY_MASK			(( 1 << NUM_SERIAL_NUM_BITS) - 1)

// view angle update types for CPlayerState::fixangle
#define FIXANGLE_NONE			0
#define FIXANGLE_ABSOLUTE		1
#define FIXANGLE_RELATIVE		2

#define FL_FAKECLIENT			(1<<7)	// Fake client, simulated server side; don't send network messages to them

enum RenderMode_t
{
	kRenderNormal = 0,		// src
	kRenderTransColor,		// c*a+dest*(1-a)
	kRenderTransTexture,	// src*a+dest*(1-a)
	kRenderGlow,			// src*a+dest -- No Z buffer checks -- Fixed size in screen space
	kRenderTransAlpha,		// src*srca+dest*(1-srca)
	kRenderTransAdd,		// src*a+dest
	kRenderEnvironmental,	// not drawn, used for environmental effects
	kRenderTransAddFrameBlend, // use a fractional frame value to blend between animation frames
	kRenderTransAlphaAdd,	// src + dest*(1-a)
	kRenderWorldGlow,		// Same as kRenderGlow but not fixed size in screen space
	kRenderNone,			// Don't render.

	kRenderModeCount,		// must be last
};

enum MoveType_t
{
	MOVETYPE_NONE = 0,      // never moves
	MOVETYPE_ISOMETRIC,     // For players -- in TF2 commander view, etc.
	MOVETYPE_WALK,          // Player only - moving on the ground
	MOVETYPE_STEP,          // gravity, special edge handling -- monsters use this
	MOVETYPE_FLY,           // No gravity, but still collides with stuff
	MOVETYPE_FLYGRAVITY,	// flies through the air + is affected by gravity
	MOVETYPE_FLY_NO_COLLIDE,// No gravity, no collision
	MOVETYPE_VPHYSICS,      // uses VPHYSICS for simulation
	MOVETYPE_PUSH,          // no clip to world, push and crush
	MOVETYPE_NOCLIP,        // No gravity, no collisions, still do velocity/avelocity
	MOVETYPE_OBSERVER,      // Observer movement, depends on player's observer mode
	MOVETYPE_CUSTOM,        // Allows the entity to describe its own physics
	MOVETYPE_TRAVERSE,      // ?
	MOVETYPE_RODEO,         // Currently rodeo'ing.
	MOVETYPE_OPERATOR,      // ?
	MOVETYPE_MELEE_LUNGE,   // Currently in melee lunge
	MOVETYPE_ZEROG          // ?
};

inline const char* const g_GameDllTargets[] = {
	"server",
	"client"
};

inline bool V_GameTargetExists(const char* const pTarget)
{
	for (size_t i = 0; i < V_ARRAYSIZE(g_GameDllTargets); i++)
	{
		if (V_strcmp(pTarget, g_GameDllTargets[i]) == NULL)
		{
			return true;
		}
	}

	return false;
}

#endif // CONST_H