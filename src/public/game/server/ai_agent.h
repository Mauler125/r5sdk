//=============================================================================//
//
// Purpose: defines the agent types for AI
//
//=============================================================================//
#ifndef AI_AGENT_H
#define AI_AGENT_H

// The traverse types determine the animations, they also determine the jump
// links that can be taken (e.g. ANIMTYPE_FRAG_DRONE can take far more jumps
// than ANIMTYPE_STALKER. This is determined during the creation of the static
// pathing data in the NavMesh.
enum TraverseAnimType_e
{
	// NAVMESH_SMALL
	ANIMTYPE_HUMAN = 0,
	ANIMTYPE_SPECTRE,
	ANIMTYPE_STALKER,
	ANIMTYPE_FRAG_DRONE,
	ANIMTYPE_PILOT,

	// NAVMESH_MED_SHORT
	ANIMTYPE_PROWLER,

	// NAVMESH_MEDIUM
	ANIMTYPE_SUPER_SPECTRE,

	// NAVMESH_LARGE
	ANIMTYPE_TITAN,

	// NAVMESH_EXTRA_LARGE
	ANIMTYPE_GOLIATH,

	// Not an anim type!
	ANIMTYPE_COUNT
};

#endif // AI_AGENT_H
