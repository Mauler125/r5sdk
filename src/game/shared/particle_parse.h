//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PARTICLE_PARSE_H
#define PARTICLE_PARSE_H

//-----------------------------------------------------------------------------
// Particle attachment methods
//-----------------------------------------------------------------------------
enum ParticleAttachment_t
{
	PATTACH_ABSORIGIN = 0x0,               // Create at absorigin, but don't follow
	PATTACH_ABSORIGIN_FOLLOW,              // Create at absorigin, and update to follow the entity
	PATTACH_ABSORIGIN_FOLLOW_NOROTATE,     // Create at absorigin, and update to follow the entity without rotation
	PATTACH_CUSTOMORIGIN,                  // Create at a custom origin, but don't follow
	PATTACH_CUSTOMORIGIN_FOLLOW,           // Create at a custom origin, follow relative position to specified entity
	PATTACH_CUSTOMORIGIN_FOLLOW_NOROTATE,  // Create at a custom origin, follow relative position to specified entity without rotation
	PATTACH_POINT,                         // Create on attachment point, but don't follow
	PATTACH_POINT_FOLLOW,                  // Create on attachment point, and update to follow the entity
	PATTACH_POINT_FOLLOW_NOROTATE,         // Create on attachment point, and update to follow the entity without rotation
	PATTACH_EYES_FOLLOW,                   // Create on eyes of the attached entity, and update to follow the entity
	PATTACH_OVERHEAD_FOLLOW,               // Create at the top of the entity's bbox
	PATTACH_WORLDORIGIN,                   // Used for control points that don't attach to an entity
	PATTACH_ROOTBONE_FOLLOW,               // Create at the root bone of the entity, and update to follow

	PATTACH_EYEANGLES_FOLLOW,
	PATTACH_CAMANGLES_FOLLOW,

	PATTACH_WAYPOINT_VECTOR,
	PATTACH_HEALTH,
	PATTACH_HEAL_TARGET,
	PATTACH_FRIENDLINESS,
	PATTACH_PLAYER_SUIT_POWER,
	PATTACH_PLAYER_GRAPPLE_POWER,
	PATTACH_PLAYER_SHARED_ENERGY,
	PATTACH_PLAYER_SHARED_ENERGY_FRACTION,
	PATTACH_WEAPON_CHARGE_FRACTION,
	PATTACH_WEAPON_SMART_AMMO_LOCK_FRACTION,
	PATTACH_WEAPON_READY_TO_FIRE_FRACTION,
	PATTACH_WEAPON_RELOAD_FRACTION,
	PATTACH_WEAPON_DRYFIRE_FRACTION,
	PATTACH_WEAPON_CLIP_AMMO_FRACTION,
	PATTACH_WEAPON_CLIP_MIN_AMMO_FRACTION,
	PATTACH_WEAPON_REMAINING_AMMO_FRACTION,
	PATTACH_WEAPON_CLIP_AMMO_MAX,
	PATTACH_WEAPON_AMMO_MAX,
	PATTACH_WEAPON_LIFETIME_SHOTS,
	PATTACH_WEAPON_AMMO_REGEN_RATE,
	PATTACH_WEAPON_STOCKPILE_REGEN_FRAC,
	PATTACH_BOOST_METER_FRACTION,
	PATTACH_GLIDE_METER_FRACTION,
	PATTACH_SHIELD_FRACTION,
	PATTACH_STATUS_EFFECT_SEVERITY,
	PATTACH_SCRIPT_NETWORK_VAR,
	PATTACH_SCRIPT_NETWORK_VAR_GLOBAL,
	PATTACH_SCRIPT_NETWORK_VAR_LOCAL_VIEW_PLAYER,
	PATTACH_FRIENDLY_TEAM_SCORE,
	PATTACH_FRIENDLY_TEAM_ROUND_SCORE,
	PATTACH_ENEMY_TEAM_SCORE,
	PATTACH_ENEMY_TEAM_ROUND_SCORE,
	PATTACH_MINIMAP_SCALE,
	PATTACH_SOUND_METER,
	PATTACH_GAME_FULLY_INSTALLED_PROGRESS,
	PATTACH_WAYPOINT_FLOAT,
	PATTACH_TASKLIST_ITEM_FLOAT,
	PATTACH_MINIMAP_ZOOM_SCALE,
	PATTACH_DEATHFIELD_DISTANCE,
	PATTACH_STATUS_EFFECT_TIME_REMAINING,
	PATTACH_BIG_MAP_ZOOM_SCALE,

	MAX_PATTACH_TYPES,
};

#endif // PARTICLE_PARSE_H
