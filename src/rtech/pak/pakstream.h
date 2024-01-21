#ifndef RTECH_PAKSTREAM_H
#define RTECH_PAKSTREAM_H
#include "rtech/ipakfile.h"

extern void Pak_OpenAssociatedStreamingFiles(PakLoadedInfo_t* const loadedInfo, PakLoadedInfo_t::StreamingInfo_t& streamInfo,
    const uint16_t fileNamesBufSize, const EPakStreamSet set);

extern void Pak_EnableEmbeddedStreamingData(PakLoadedInfo_t* const loadedInfo, PakLoadedInfo_t::StreamingInfo_t& streamInfo);
extern void Pak_LoadStreamingData(PakLoadedInfo_t* const loadedInfo);

// bool set as int64.
inline int64_t* g_pUseAssetStreamingSystem = nullptr;
inline int64_t* g_pNumStreamableAssets = nullptr;
inline float* g_pStreamingDownloadProgress = nullptr;

// inlines
inline void(*v_Pak_IncrementStreamingAssetCount)(void);
inline void(*v_Pak_DecrementStreamingAssetCount)(void);

inline bool Pak_StreamingEnabled()          { return *g_pUseAssetStreamingSystem != NULL; }
inline int64_t Pak_GetNumStreamableAssets() { return *g_pNumStreamableAssets; }

inline float Pak_GetStreamingDownloadProgress() { return *g_pStreamingDownloadProgress; }
inline bool Pak_StreamingDownloadFinished()     { return Pak_GetStreamingDownloadProgress() == 1.0f; }

class V_PakStream : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Pak_IncrementStreamingAssetCount", v_Pak_IncrementStreamingAssetCount);
		LogFunAdr("Pak_DecrementStreamingAssetCount", v_Pak_DecrementStreamingAssetCount);

		LogVarAdr("g_useAssetStreamingSystem", g_pUseAssetStreamingSystem);
		LogVarAdr("g_numStreamableAssets", g_pNumStreamableAssets);
		LogVarAdr("g_streamingDownloadProgress", g_pStreamingDownloadProgress);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("F0 48 FF 05 ?? ?? ?? ?? C3").GetPtr(v_Pak_IncrementStreamingAssetCount);
		g_GameDll.FindPatternSIMD("F0 48 FF 0D ?? ?? ?? ?? C3").GetPtr(v_Pak_DecrementStreamingAssetCount);
	}
	virtual void GetVar(void) const
	{
		extern PakHandle_t(*v_Pak_Initialize)(int mode);
		CMemory(v_Pak_Initialize).Offset(0x120).FindPatternSelf("48 89 05").ResolveRelativeAddressSelf(3, 7).GetPtr(g_pUseAssetStreamingSystem);

		g_GameDll.FindPatternSIMD("F3 0F 10 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC 48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ??")
			.ResolveRelativeAddress(0x4, 0x8).GetPtr(g_pStreamingDownloadProgress);

		CMemory(v_Pak_IncrementStreamingAssetCount).ResolveRelativeAddress(4, 8).GetPtr(g_pNumStreamableAssets); // 167ED7BB8
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};

#endif // RTECH_PAKSTREAM_H
