#pragma once
#include "public/edict.h"
#include "public/eiface.h"

/* ==== CVENGINESERVER ================================================================================================================================================== */
inline CMemory p_IVEngineServer__PersistenceAvailable;
inline auto IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void* entidx, int clientidx)>();

//inline CMemory p_RunFrameServer;
//inline auto v_RunFrameServer = p_RunFrameServer.RCast<void(*)(double flFrameTime, bool bRunOverlays, bool bUniformUpdate)>();

inline bool* g_bDedicated = nullptr;

///////////////////////////////////////////////////////////////////////////////
void IVEngineServer_Attach();
void IVEngineServer_Detach();

///////////////////////////////////////////////////////////////////////////////

struct ServerPlayer_t
{
	ServerPlayer_t(void)
		: m_flCurrentNetProcessTime(0.0)
		, m_flLastNetProcessTime(0.0)
		, m_flStringCommandQuotaTimeStart(0.0)
		, m_nStringCommandQuotaCount(0)
		, m_bPersistenceEnabled(false)
	{}
	inline void Reset(void)
	{
		m_flCurrentNetProcessTime = 0.0;
		m_flLastNetProcessTime = 0.0;
		m_flStringCommandQuotaTimeStart = 0.0;
		m_nStringCommandQuotaCount = 0;
		m_bPersistenceEnabled = false;
	}

	double m_flCurrentNetProcessTime;
	double m_flLastNetProcessTime;
	double m_flStringCommandQuotaTimeStart;
	int m_nStringCommandQuotaCount;
	bool m_bPersistenceEnabled;
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
		spdlog::debug("| FUN: CVEngineServer::PersistenceAvailable : {:#18x} |\n", p_IVEngineServer__PersistenceAvailable.GetPtr());
		//spdlog::debug("| FUN: RunFrameServer                       : {:#18x} |\n", p_RunFrameServer.GetPtr());
		spdlog::debug("| VAR: g_bDedicated                         : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bDedicated));
		spdlog::debug("| VAR: g_pEngineServerVFTable               : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pEngineServerVFTable));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_IVEngineServer__PersistenceAvailable = g_GameDll.FindPatternSIMD("3B 15 ?? ?? ?? ?? 7D 33");
//		p_RunFrameServer                       = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 30 0F 29 74 24 ?? 48 8D 0D ?? ?? ?? ??");

		IVEngineServer__PersistenceAvailable = p_IVEngineServer__PersistenceAvailable.RCast<bool (*)(void*, int)>();       /*3B 15 ?? ?? ?? ?? 7D 33*/
//		v_RunFrameServer                     = p_RunFrameServer.RCast<void(*)(double, bool, bool)>();                        /*48 89 5C 24 ?? 57 48 83 EC 30 0F 29 74 24 ?? 48 8D 0D ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const
	{
		CMemory pEngineServerVFTable = g_GameDll.GetVirtualMethodTable(".?AVCVEngineServer@@", 0);

		g_pEngineServerVFTable = pEngineServerVFTable.RCast<CVEngineServer*>();
		g_bDedicated = pEngineServerVFTable.WalkVTableSelf(3).DerefSelf().ResolveRelativeAddress(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineServer);
