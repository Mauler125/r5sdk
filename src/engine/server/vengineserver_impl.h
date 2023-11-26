#pragma once
#include "public/edict.h"
#include "public/eiface.h"

/* ==== CVENGINESERVER ================================================================================================================================================== */
inline CMemory p_IVEngineServer__PersistenceAvailable;
inline bool(*IVEngineServer__PersistenceAvailable)(void* entidx, int clientidx);

inline bool* m_bIsDedicated = nullptr;

///////////////////////////////////////////////////////////////////////////////

struct ServerPlayer_t
{
	ServerPlayer_t(void)
	{
		Reset();
	}
	inline void Reset(void)
	{
		m_flCurrentNetProcessTime = 0.0;
		m_flLastNetProcessTime = 0.0;
		m_flStringCommandQuotaTimeStart = 0.0;
		m_nStringCommandQuotaCount = NULL;
		m_bInitialConVarsSet = false;
	}

	double m_flCurrentNetProcessTime;
	double m_flLastNetProcessTime;
	double m_flStringCommandQuotaTimeStart;
	int m_nStringCommandQuotaCount;
	bool m_bInitialConVarsSet;
};

extern ServerPlayer_t g_ServerPlayer[MAX_PLAYERS];

class CVEngineServer : public IVEngineServer
{
public:
	static bool PersistenceAvailable(void* entidx, int clientidx);
	// Implementation in GameDLL.
};
extern CVEngineServer* g_pEngineServer;
extern IVEngineServer* g_pEngineServerVFTable;

///////////////////////////////////////////////////////////////////////////////
class HVEngineServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CVEngineServer::`vftable'", reinterpret_cast<uintptr_t>(g_pEngineServerVFTable));
		LogFunAdr("CVEngineServer::PersistenceAvailable", p_IVEngineServer__PersistenceAvailable.GetPtr());
		LogVarAdr("m_bIsDedicated", reinterpret_cast<uintptr_t>(m_bIsDedicated)); // !TODO: part of CServer!
	}
	virtual void GetFun(void) const
	{
		p_IVEngineServer__PersistenceAvailable = g_GameDll.FindPatternSIMD("3B 15 ?? ?? ?? ?? 7D 33");
		IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void*, int)>();
	}
	virtual void GetVar(void) const
	{
		CMemory pEngineServerVFTable = g_GameDll.GetVirtualMethodTable(".?AVCVEngineServer@@", 0);

		g_pEngineServerVFTable = pEngineServerVFTable.RCast<CVEngineServer*>();
		m_bIsDedicated = pEngineServerVFTable.WalkVTableSelf(3).DerefSelf().ResolveRelativeAddress(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
