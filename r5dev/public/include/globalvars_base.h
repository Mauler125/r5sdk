#pragma once
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
enum GameMode_t
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

	float           m_nUnkTime;
	float           m_rRealTime;                     // Absolute time (per frame still - Use Plat_FloatTime() for a high precision real time.
	int             m_nFrameCount;                   // Absolute frame counter - continues to increase even if game is paused - never resets.
	float           m_fAbsoluteFrameTime;            // Non-paused frametime
	float           m_fAbsoluteFrameStartTimeStdDev;
	float           m_fFrameTime;                    // Time spent on last server or client frame (has nothing to do with think intervals) (Also empty on dedicated)
	float           m_fCurTime;                      
	char            m_nPad0[24];                     // All unknown.
	int             m_nMaxClients;                   // Current maxplayers setting
	int             m_nMaxMilesAudioQueues_Maybe;    // Only used on the server.
	GameMode_t      m_nGameMode;;                    // 1 (MP) 2 (PVE) 3 (SP)
	int             m_nTickCount;                    // Simulation ticks - does not increase when game is paused - resets on restart.
	int             m_nUnk0;
	int             m_nUnk1;

	// The following seem to be mainly used in c:\depot\r5launch\src\engine\client\clientstate.cpp.
	int             m_nUnk2;             
	int             m_nUnk3;                        // Seems to be used on client only.
	int             m_nUnk4;                        // Seems to be used on client only.
};