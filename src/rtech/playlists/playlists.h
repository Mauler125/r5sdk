#ifndef RTECH_PLAYLISTS_H
#define RTECH_PLAYLISTS_H
#include "tier1/keyvalues.h"

///////////////////////////////////////////////////////////////////////////////
void Playlists_SDKInit(void);
bool Playlists_Load(const char* pszPlaylist);
bool Playlists_Parse(const char* pszPlaylist);

///////////////////////////////////////////////////////////////////////////////
inline bool(*v_Playlists_Load)(const char* pszPlaylist);
inline bool(*v_Playlists_Parse)(const char* pszPlaylist);
inline const char* (*v_Playlists_GetCurrent)(void);
inline void(*v_Playlists_Download_f)(void);

extern KeyValues** g_pPlaylistKeyValues;

extern vector<string> g_vAllPlaylists;
extern std::mutex g_PlaylistsVecMutex;

///////////////////////////////////////////////////////////////////////////////
class VPlaylists : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Playlists_Load", v_Playlists_Load);
		LogFunAdr("Playlists_Parse", v_Playlists_Parse);
		LogFunAdr("Playlists_GetCurrent", v_Playlists_GetCurrent);
		LogFunAdr("Playlists_Download_f", v_Playlists_Download_f);
		LogVarAdr("g_pPlaylistKeyValues", g_pPlaylistKeyValues);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 40 48 8B F1").GetPtr(v_Playlists_Load);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 0C").FollowNearCallSelf().GetPtr(v_Playlists_Parse);
		g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? 48 85 C0 75 08 48 8D 05 ?? ?? ?? ?? C3 0F B7 50 2A").GetPtr(v_Playlists_GetCurrent);
		g_GameDll.FindPatternSIMD("33 C9 C6 05 ?? ?? ?? ?? ?? E9 ?? ?? ?? ??").GetPtr(v_Playlists_Download_f);
	}
	virtual void GetVar(void) const
	{
		g_pPlaylistKeyValues = g_GameDll.FindPatternSIMD("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 E8 B4")
			.FindPatternSelf("48 8B 0D", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<KeyValues**>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // RTECH_PLAYLISTS_H
