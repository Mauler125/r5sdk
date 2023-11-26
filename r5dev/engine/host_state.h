#pragma once
#include "mathlib/vector.h"

enum class HostStates_t : int
{
	HS_NEW_GAME        = 0x0,
	HS_LOAD_GAME       = 0x1,
	HS_CHANGE_LEVEL_SP = 0x2,
	HS_CHANGE_LEVEL_MP = 0x3,
	HS_RUN             = 0x4,
	HS_GAME_SHUTDOWN   = 0x5,
	HS_SHUTDOWN        = 0x6,
	HS_RESTART         = 0x7,
};

class CHostState
{
public:
	static void FrameUpdate(CHostState* pHostState, double flCurrentTime, float flFrameTime);
	void LoadConfig(void) const;

	void Init(void);
	void Setup(void);
	void Think(void) const;

	void SetState(const HostStates_t newState);
	void GameShutDown(void);

	void State_NewGame(void);
	void State_ChangeLevelSP(void);
	void State_ChangeLevelMP(void);

	void ResetLevelName(void);

public:
	HostStates_t m_iCurrentState;                    //0x0000
	HostStates_t m_iNextState;                       //0x0004
	Vector3D     m_vecLocation;                      //0x0008
	QAngle       m_angLocation;                      //0x0014
	char         m_levelName[MAX_MAP_NAME_HOST];     //0x0020
	char         m_mapGroupName[256];                //0x0060
	char         m_landMarkName[256];                //0x0160
	float        m_flShortFrameTime;                 //0x0260
	bool         m_bActiveGame;                      //0x0264
	bool         m_bRememberLocation;                //0x0265
	bool         m_bBackgroundLevel;                 //0x0266
	bool         m_bWaitingForConnection;            //0x0267
	bool         m_bSplitScreenConnect;              //0x0268
	bool         m_bGameHasShutDownAndFlushedMemory; //0x0269
	HostStates_t m_iServerState;                     //0x026C
};

/* ==== CHOSTSTATE ====================================================================================================================================================== */
inline CMemory p_CHostState_FrameUpdate;
inline void(*CHostState_FrameUpdate)(CHostState* pHostState, double flCurrentTime, float flFrameTime);

inline CMemory p_CHostState_State_Run;
inline void(*CHostState_State_Run)(HostStates_t* pState, double flCurrentTime, float flFrameTime);

inline CMemory p_CHostState_State_GameShutDown;
inline void(*CHostState_State_GameShutDown)(CHostState* thisptr);

inline CMemory p_HostState_ChangeLevelMP;
inline void(*v_HostState_ChangeLevelMP)(char const* pNewLevel, char const* pLandmarkName);

///////////////////////////////////////////////////////////////////////////////
extern CHostState* g_pHostState;

///////////////////////////////////////////////////////////////////////////////
class VHostState : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CHostState::FrameUpdate", p_CHostState_FrameUpdate.GetPtr());
		LogFunAdr("CHostState::State_Run", p_CHostState_State_Run.GetPtr());
		LogFunAdr("CHostState::State_GameShutDown", p_CHostState_State_GameShutDown.GetPtr());
		LogFunAdr("HostState_ChangeLevelMP", p_HostState_ChangeLevelMP.GetPtr());
		LogVarAdr("g_pHostState", reinterpret_cast<uintptr_t>(g_pHostState));
	}
	virtual void GetFun(void) const
	{
		p_CHostState_FrameUpdate  = g_GameDll.FindPatternSIMD("48 89 5C 24 08 48 89 6C 24 20 F3 0F 11 54 24 18");
		p_CHostState_State_Run    = g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 C8 45 33 E4");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CHostState_State_GameShutDown = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 56 48 83 EC 20 8B 05 ?? ?? ?? ?? 48 8B F1");
#elif defined (GAMEDLL_S2)
		p_CHostState_State_GameShutDown = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B 05 ?? ?? ?? ?? 33 FF 48 8B F1");
#elif defined (GAMEDLL_S3)
		p_CHostState_State_GameShutDown = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ??");
#endif
		p_HostState_ChangeLevelMP = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 48 8B F2 8B 0D ?? ?? ?? ??");

		CHostState_FrameUpdate        = p_CHostState_FrameUpdate.RCast<void(*)(CHostState*, double, float)>(); /*48 89 5C 24 08 48 89 6C 24 20 F3 0F 11 54 24 18*/
		CHostState_State_Run          = p_CHostState_State_Run.RCast<void(*)(HostStates_t*, double, float)>(); /*48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 C8 45 33 E4*/
		CHostState_State_GameShutDown = p_CHostState_State_GameShutDown.RCast<void(*)(CHostState* thisptr)>(); /*48 89 5C 24 ?? 57 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ??*/
		v_HostState_ChangeLevelMP     = p_HostState_ChangeLevelMP.RCast<void(*)(char const*, char const*)>();  /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9 48 8B F2 8B 0D ? ? ? ?*/
	}
	virtual void GetVar(void) const
	{
		g_pHostState = p_CHostState_FrameUpdate.FindPattern("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CHostState*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
