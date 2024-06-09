#pragma once
#include "miles_types.h"

/* ==== WASAPI THREAD SERVICE =========================================================================================================================================== */
inline void(*v_AIL_LogFunc)(int64_t nLogLevel, const char* pszMessage);
inline bool(*v_Miles_Initialize)();
inline void(*v_MilesQueueEventRun)(Miles::Queue*, const char*);
inline void(*v_MilesBankPatch)(Miles::Bank*, char*, char*);
inline unsigned int (*v_MilesSampleSetSourceRaw)(__int64 a1, __int64 a2, unsigned int a3, int a4, unsigned __int16 a5, bool a6);
inline void(*v_CSOM_AddEventToQueue)(const char* eventName);

struct MilesBankList_t
{
	char banks[64][16];
	int bankCount;
};

struct MilesGlobalState_t
{
	char gap0[24];
	bool mismatchedBuildTag;
	char gap19[63];
	uintptr_t queuedEventHash;
	char gap60[4];
	Vector3D queuedSoundPosition;
	char gap70[24];
	float soundMasterVolume;
	char gap8c[28];
	void* samplesXlogType;
	char gapB0[8];
	void* dumpXlogType;
	char gapC0[48];
	void* driver;
	void* queue;
	char gap100[40];
	MilesBankList_t bankList;
	char gap52c[4];
	void* loadedBanks[16];
	char gap5b0[290448];
	HANDLE milesInitializedEvent;
	HANDLE milesThread;
	char gap47450[272];
	char milesOutputBuffer[1024];
	char unk[96];
};

inline MilesGlobalState_t* g_milesGlobals;

///////////////////////////////////////////////////////////////////////////////
class MilesCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("AIL_LogFunc", v_AIL_LogFunc);
		LogFunAdr("Miles_Initialize", v_Miles_Initialize);
		LogFunAdr("MilesQueueEventRun", v_MilesQueueEventRun);
		LogFunAdr("MilesBankPatch", v_MilesBankPatch);
		LogFunAdr("MilesSampleSetSourceRaw", v_MilesSampleSetSourceRaw);
		LogFunAdr("CSOM_AddEventToQueue", v_CSOM_AddEventToQueue);
		LogVarAdr("g_milesGlobals", g_milesGlobals);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B DA 48 8D 15 ?? ?? ?? ??").GetPtr(v_AIL_LogFunc);
		g_GameDll.FindPatternSIMD("0F B6 11 4C 8B C1").GetPtr(v_CSOM_AddEventToQueue);
		
		CMemory milesInitializeFunc = g_GameDll.FindPatternSIMD("40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");
		milesInitializeFunc.GetPtr(v_Miles_Initialize);

		g_milesGlobals = milesInitializeFunc.FindPatternSelf("48 8D", CMemory::Direction::DOWN, 0x50).ResolveRelativeAddressSelf(0x3, 0x7).RCast<MilesGlobalState_t*>();

		g_RadAudioSystemDll.GetExportedSymbol("MilesQueueEventRun").GetPtr(v_MilesQueueEventRun);
		g_RadAudioSystemDll.GetExportedSymbol("MilesBankPatch").GetPtr(v_MilesBankPatch);
		g_RadAudioSystemDll.GetExportedSymbol("MilesSampleSetSourceRaw").GetPtr(v_MilesSampleSetSourceRaw);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
