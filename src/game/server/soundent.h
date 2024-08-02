//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//-----------------------------------------------------------------------------
// Soundent.h - the entity that spawns when the world 
// spawns, and handles the world's active and free sound
// lists.
//=============================================================================//
#ifndef SOUNDENT_H
#define SOUNDENT_H

#ifdef _WIN32
#pragma once
#endif

#include "game/shared/predictioncopy.h"

enum
{
	SOUND_NONE              = 0,
	SOUND_COMBAT            = 0x00000001,
	SOUND_PLAYER            = 0x00000002,
	SOUND_WORLD             = 0x00000004, // this might be wrong, wasn't in the registration in [r5apex_ds + E42FD0]
	SOUND_BULLET_IMPACT     = 0x00000008,
	SOUND_PAINDEATH         = 0x00000010,

	// Anything below here is unconfirmed for r5 !!!
	SOUND_CARCASS           = 0x00000020,
	SOUND_MEAT              = 0x00000040,
	SOUND_GARBAGE           = 0x00000080,
	SOUND_THUMPER           = 0x00000100, // keeps certain creatures at bay
	SOUND_BUGBAIT           = 0x00000200, // gets the antlion's attention
	SOUND_PHYSICS_DANGER    = 0x00000400,
	SOUND_DANGER_SNIPERONLY = 0x00000800, // only scares the sniper NPC.
	SOUND_MOVE_AWAY         = 0x00001000,
	SOUND_PLAYER_VEHICLE    = 0x00002000,
	SOUND_READINESS_LOW     = 0x00004000, // Changes listener's readiness (Player Companion only)
	SOUND_READINESS_MEDIUM  = 0x00008000,
	SOUND_READINESS_HIGH    = 0x00010000,

	SOUND_AI_THREATS = SOUND_COMBAT | SOUND_BULLET_IMPACT,

	// Contexts begin here.
	SOUND_CONTEXT_FROM_SNIPER       = 0x00100000, // additional context for SOUND_DANGER
	SOUND_CONTEXT_GUNFIRE           = 0x00200000, // Added to SOUND_COMBAT
	SOUND_CONTEXT_MORTAR            = 0x00400000, // Explosion going to happen here.
	SOUND_CONTEXT_COMBINE_ONLY      = 0x00800000, // Only combine can hear sounds marked this way
	SOUND_CONTEXT_REACT_TO_SOURCE   = 0x01000000, // React to sound source's origin, not sound's location
	SOUND_CONTEXT_EXPLOSION         = 0x02000000, // Context added to SOUND_COMBAT, usually.
	SOUND_CONTEXT_EXCLUDE_COMBINE   = 0x04000000, // Combine do NOT hear this
	SOUND_CONTEXT_DANGER_APPROACH   = 0x08000000, // Treat as a normal danger sound if you see the source, otherwise turn to face source.
	SOUND_CONTEXT_ALLIES_ONLY       = 0x10000000, // Only player allies can hear this sound
	SOUND_CONTEXT_PLAYER_VEHICLE    = 0x20000000, // HACK: need this because we're not treating the SOUND_xxx values as true bit values! See switch in OnListened.
	SOUND_CONTEXT_FROM_FIRE         = 0x40000000, // A fire is nearby
	SOUND_CONTEXT_FOLLOW_OWNER      = 0x80000000, // The sound origin is at the owner

	ALL_CONTEXTS            = 0xFFF00000,
	ALL_SCENTS              = SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE,
	ALL_SOUNDS              = 0x000FFFFF & ~ALL_SCENTS,
};

//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
	//DECLARE_SIMPLE_DATADESC();
public:
	bool	DoesSoundExpire() const;
	float	SoundExpirationTime() const;
	void	SetSoundOrigin(const Vector3D& vecOrigin) { m_vecOrigin = vecOrigin; }
	//const	Vector3D& GetSoundOrigin(void);
	//const	Vector3D& GetSoundReactOrigin(void);
	//bool	FIsSound(void);
	//bool	FIsScent(void);
	bool	IsSoundType(int nSoundFlags) const;
	int		SoundType() const;
	int		SoundContext() const;
	int		SoundTypeNoContext() const;
	int		NextSound() const;
	//void	Reset(void);
	int		SoundChannel(void) const;
	//bool	ValidateOwner() const;

	EHANDLE  m_hOwner;             // sound's owner.
	EHANDLE  m_hTarget;            // Sounds's target - an odd concept. For a gunfire sound, the target is the entity being fired at.
	float    m_audibleRadius;
	float    m_expireTime;         // When the sound should be purged from the list.
	int      m_ownerChannelIndex;
	int      m_iType;              // What type of sound this is
	Vector3D m_vecOrigin;          // Sound's location in space.
	edict_t  m_iNext;              // Temporary link that NPCs use to build a list of audible sounds.
	bool     m_bReserved;
	bool     m_ownerMustExist;
};

inline bool CSound::DoesSoundExpire() const
{
	return m_expireTime > 0.0f;
}

inline float CSound::SoundExpirationTime() const
{
	return m_expireTime;
}

inline bool CSound::IsSoundType(int nSoundFlags) const
{
	return (m_iType & nSoundFlags) != 0;
}

inline int CSound::SoundType() const
{
	return m_iType;
}

inline int CSound::SoundContext() const
{
	return m_iType & ALL_CONTEXTS;
}

inline int CSound::SoundTypeNoContext() const
{
	return m_iType & ~ALL_CONTEXTS;
}

inline int CSound::NextSound() const
{
	return m_iNext;
}

inline int CSound::SoundChannel(void) const
{
	return m_ownerChannelIndex;
}

#endif //SOUNDENT_H
