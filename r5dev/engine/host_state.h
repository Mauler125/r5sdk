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

	FORCEINLINE static void FrameUpdate(CHostState* rcx, void* rdx, float time);
	FORCEINLINE void LoadConfig(void) const;

	FORCEINLINE void Init(void);
	FORCEINLINE void Setup(void);
	FORCEINLINE void Think(void) const;

	FORCEINLINE void GameShutDown(void);
	FORCEINLINE void UnloadPakFile(void) const;

	FORCEINLINE void State_NewGame(void);
	FORCEINLINE void State_ChangeLevelSP(void);
	FORCEINLINE void State_ChangeLevelMP(void);

	FORCEINLINE void ResetLevelName(void);
	FORCEINLINE bool LevelHasChanged(void) const;

public:
	HostStates_t m_iCurrentState;                    //0x0000
	HostStates_t m_iNextState;                       //0x0004
	Vector3      m_vecLocation;                      //0x0008
	QAngle       m_angLocation;                      //0x0014
	char         m_levelName[64];                    //0x0020
	char         m_mapGroupName[256];                //0x0060
	char         m_landMarkName[256];                //0x0160
	float        m_flShortFrameTime;                 //0x0260
	bool         m_bActiveGame;                      //0x0264
	bool         m_bRememberLocation;                //0x0265
	bool         m_bBackgroundLevel;                 //0x0266
	bool         m_bWaitingForConnection;            //0x0267
	bool         m_bSplitScreenConnect;              //0x0268
	bool         m_bGameHasShutDownAndFlushedMemory; //0x0269
	bool         m_bWorkshopMapDownloadPending;      //0x026A
};

/* ==== CHOSTSTATE ====================================================================================================================================================== */
inline CMemory p_CHostState_FrameUpdate;
inline auto CHostState_FrameUpdate = p_CHostState_FrameUpdate.RCast<void(*)(CHostState* rcx, void* rdx, float time)>();

inline CMemory p_CHostState_State_Run;
inline auto CHostState_State_Run = p_CHostState_State_Run.RCast<void(*)(HostStates_t* pState, void* pUnused, float flFrameTime)>();

inline CMemory p_CHostState_State_GameShutDown;
inline auto CHostState_State_GameShutDown = p_CHostState_State_GameShutDown.RCast<void(*)(CHostState* thisptr)>();

extern bool g_bLevelResourceInitialized;
extern bool g_bBasePaksInitialized;
///////////////////////////////////////////////////////////////////////////////
void CHostState_Attach();
void CHostState_Detach();

///////////////////////////////////////////////////////////////////////////////
extern CHostState* g_pHostState;

///////////////////////////////////////////////////////////////////////////////
class HHostState : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CHostState::FrameUpdate              : 0x" << std::hex << std::uppercase << p_CHostState_FrameUpdate.GetPtr()        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHostState::State_Run                : 0x" << std::hex << std::uppercase << p_CHostState_State_Run.GetPtr()          << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CHostState::State_GameShutDown       : 0x" << std::hex << std::uppercase << p_CHostState_State_GameShutDown.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pHostState                         : 0x" << std::hex << std::uppercase << g_pHostState                             << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_CHostState_FrameUpdate  = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x20\xF3\x0F\x11\x54\x24\x18"), "xxxxxxxxxxxxxxxx");
		p_CHostState_State_Run    = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x70\x18\x48\x89\x78\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xC8\x45\x33\xE4"), "xxxxxxxxxxxxxxxxxxxxxxxxxxx????xxx????xxxxxxx");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CHostState_State_GameShutDown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x56\x48\x83\xEC\x20\x8B\x05\x00\x00\x00\x00\x48\x8B\xF1"), "xxxx?xxxxxxx????xxx");
#elif defined (GAMEDLL_S2)
		p_CHostState_State_GameShutDown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x8B\x05\x00\x00\x00\x00\x33\xFF\x48\x8B\xF1"), "xxxx?xxxx?xxxxxxx????xxxxx");
#elif defined (GAMEDLL_S3)
		p_CHostState_State_GameShutDown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xD9\xE8\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00"), "xxxx?xxxxxxxxx????xxx????");
#endif

		CHostState_FrameUpdate        = p_CHostState_FrameUpdate.RCast<void(*)(CHostState*, void*, float)>();  /*48 89 5C 24 08 48 89 6C 24 20 F3 0F 11 54 24 18*/
		CHostState_State_Run          = p_CHostState_State_Run.RCast<void(*)(HostStates_t*, void*, float)>();  /*48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 C8 45 33 E4*/
		CHostState_State_GameShutDown = p_CHostState_State_GameShutDown.RCast<void(*)(CHostState* thisptr)>(); /*48 89 5C 24 ?? 57 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ??*/

	}
	virtual void GetVar(void) const
	{
		g_pHostState = p_CHostState_FrameUpdate.FindPattern("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CHostState*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HHostState);
