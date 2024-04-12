#pragma once
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
enum class GameMode_t
{
	NO_MODE = 0,
	MP_MODE,
	PVE_MODE,
	SP_MODE,
};

//-----------------------------------------------------------------------------
// Purpose: Global variables used by shared code
//-----------------------------------------------------------------------------
class CGlobalVarsBase
{
public:
	float m_nUnkTime;
	float m_rRealTime; // Absolute time (per frame still - Use Plat_FloatTime() for a high precision real time.
	int m_nFrameCount; // Absolute frame counter - continues to increase even if game is paused - never resets.
	float m_flAbsoluteFrameTime; // Non-paused frametime.
	float m_flCurTime;

	// These seem to be mainly used in c:\depot\r5launch\src\engine\client\clientstate.cpp.
	float m_flCurTimeUnknown0; // Empty on server.
	float m_flCurTimeUnknown1; // Empty on server.
	float m_flCurTimeUnknown2; // Empty on server.
	float m_flLastFrameTimeSincePause; // Last frame time since pause, empty on server.
	float m_flCurTimeUnknown3; // Empty on server.
	float m_flLastCurTimeSincePause;   // Last current time since pause, empty on server.
	float m_flUnknown4;

	float m_flFrameTime; // Time spent on last server or client frame (has nothing to do with think intervals)
	int m_nMaxPlayers;   // Max internal player entities.
	int m_nMaxClients;   // Max players as specified in the playlists file.
	GameMode_t m_nGameMode; // 1 (MP) 2 (PVE) 3 (SP)
	int m_nTickCount;    // Simulation ticks - resets on restart.
	float m_flTickInterval;

	int m_nUnk1;
	int m_nUnk2;

	// 100 (i.e., tickcount is rounded down to this base and then the "delta" from this base is networked
	int m_nTimestampNetworkingBase;
	// 32 (entindex() % nTimestampRandomizeWindow ) is subtracted from gpGlobals->tickcount to set the networking basis, prevents
	// all of the entities from forcing a new PackedEntity on the same tick (i.e., prevents them from getting lockstepped on this)
	int m_nTimestampRandomizeWindow;
};
