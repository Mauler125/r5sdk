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
	CGlobalVarsBase(const bool bIsClient);

	// This can be used to filter debug output or to catch the client or server in the act.
	bool IsClient() const;

	// for encoding m_flSimulationTime, m_flAnimTime
	int GetNetworkBase(const int nTick, const int nEntity);

public:
	float m_nUnkTime;
	float realTime; // Absolute time (per frame still - Use Plat_FloatTime() for a high precision real time.
	int frameCount; // Absolute frame counter - continues to increase even if game is paused - never resets.
	float absoluteFrameTime; // Non-paused frametime.

	// Current time 
	//
	// On the client, this (along with tickcount) takes a different meaning based on what
	// piece of code you're in:
	// 
	//   - While receiving network packets (like in PreDataUpdate/PostDataUpdate and proxies),
	//     this is set to the SERVER TICKCOUNT for that packet. There is no interval between
	//     the server ticks.
	//     [server_current_Tick * tick_interval]
	//
	//   - While rendering, this is the exact client clock 
	//     [client_current_tick * tick_interval + interpolation_amount]
	//
	//   - During prediction, this is based on the client's current tick:
	//     [client_current_tick * tick_interval]
	float curTime;

	// These seem to be mainly used in c:\depot\r5launch\src\engine\client\clientstate.cpp.
	float m_flCurTimeUnknown0; // Empty on server.
	float m_flCurTimeUnknown1; // Empty on server.
	float m_flCurTimeUnknown2; // Empty on server.
	float lastFrameTimeSincePause; // Last frame time since pause, empty on server.
	float m_flCurTimeUnknown3; // Empty on server.
	float exactCurTime;   // Empty on server.
	float m_flUnknown4;

	float frameTime;  // Time spent on last server or client frame (has nothing to do with think intervals)
	int maxPlayers;   // Max internal player entities.
	int maxClients;   // Max players as specified in the playlists file.
	GameMode_t gameMode; // 1 (MP) 2 (PVE) 3 (SP)
	int tickCount;    // Simulation ticks - resets on restart.
	float tickInterval;

	int m_nUnk1;

private:
	// Set to true in client code.
	bool			m_bClient;
	// 100 (i.e., tickcount is rounded down to this base and then the "delta" from this base is networked
	int m_nTimestampNetworkingBase;
	// 32 (entindex() % nTimestampRandomizeWindow ) is subtracted from gpGlobals->tickcount to set the networking basis, prevents
	// all of the entities from forcing a new PackedEntity on the same tick (i.e., prevents them from getting lockstepped on this)
	int m_nTimestampRandomizeWindow;
};

inline int CGlobalVarsBase::GetNetworkBase(const int nTick, const int nEntity)
{
	const int nEntityMod = nEntity % m_nTimestampRandomizeWindow;
	const int nBaseTick = m_nTimestampNetworkingBase * (int)((nTick - nEntityMod) / m_nTimestampNetworkingBase);
	return nBaseTick;
}

inline CGlobalVarsBase::CGlobalVarsBase(const bool bIsClient) :
	m_bClient(bIsClient),
	m_nTimestampNetworkingBase(100),
	m_nTimestampRandomizeWindow(32)
{
}

inline bool CGlobalVarsBase::IsClient() const
{
	return m_bClient;
}
