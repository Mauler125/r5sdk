#pragma once

#define MAKE_3_BYTES_FROM_1_AND_2( x1, x2 ) (( (( uint16_t )x2) << 8 ) | (uint8_t)(x1))
extern vector<string> g_szAllPlaylists;

//---------------------------------------------------------------------------------
// Purpose: Forward declarations
//---------------------------------------------------------------------------------
class KeyValues;

/* ==== KEYVALUES ======================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline ADDRESS p_KeyValues_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\xC7\x44\x24\x30\xFF\xFF\xFF"), "xxxxxxxxxxxxxxxx");
inline auto KeyValues_Init = p_KeyValues_Init.RCast<void* (*)(KeyValues* thisptr, const char* pSymbol, int64_t a3, bool bCreate)>(); /*40 53 48 83 EC 20 48 8B D9 C7 44 24 30 FF FF FF*/

inline ADDRESS p_KeyValues_FindKey = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x10\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x20\x01\x00\x00\x45"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto KeyValues_FindKey = p_KeyValues_FindKey.RCast<void* (*)(KeyValues* thisptr, const char* pkeyName, bool bCreate)>(); /*48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 81 EC 20 01 00 00 45*/

inline ADDRESS p_KeyValues_GetCurrentPlaylist = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x75\x08\x48\x8D\x05\x00\x00\x00\x00"), "xxx????xxxxxxxx????");
inline auto KeyValues_GetCurrentPlaylist = p_KeyValues_GetCurrentPlaylist.RCast<const char* (*)(void)>(); /*48 8B 0D ? ? ? ? 48 85 C9 75 08 48 8D 05 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline ADDRESS p_KeyValues_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x01\x48\x8B\xD9\x4C\x8B\xC2"), "xxxxxxxxx???xxxxxxx"); /*40 53 48 83 EC 20 48 8B 05 ?? ?? ?? 01 48 8B D9 4C 8B C2*/
inline auto KeyValues_Init = p_KeyValues_Init.RCast<void* (*)(KeyValues* thisptr, const char* pSymbol, int64_t a3, bool bCreate)>();

inline ADDRESS p_KeyValues_FindKey = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x57\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x45"), "xxxxxxxx????x");
inline auto KeyValues_FindKey = p_KeyValues_FindKey.RCast<void* (*)(KeyValues* thisptr, const char* pkeyName, bool bCreate)>(); /*40 56 57 41 57 48 81 EC 30 01 00 00 45 0F B6 F8*/

inline ADDRESS p_KeyValues_GetCurrentPlaylist = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x08\x48\x8D\x05\x00\x00\x00\x00\xC3\x0F\xB7\x50\x2A"), "xxx????xxxxxxxx????xxxxx");
inline auto KeyValues_GetCurrentPlaylist = p_KeyValues_GetCurrentPlaylist.RCast<const char* (*)(void)>(); /*48 8B 05 ? ? ? ? 48 85 C0 75 08 48 8D 05 ? ? ? ? C3 0F B7 50 2A*/
#endif
inline ADDRESS p_KeyValues_LoadPlaylist = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x74\x0C"), "x????xx?????xx").FollowNearCallSelf().GetPtr();
inline auto KeyValues_LoadPlaylist = p_KeyValues_LoadPlaylist.RCast<bool (*)(const char* pszPlaylist)>(); /*E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 0C*/

enum KeyValuesTypes
{
	TYPE_NONE              = 0x0,
	TYPE_STRING            = 0x1,
	TYPE_INT               = 0x2,
	TYPE_FLOAT             = 0x3,
	TYPE_PTR               = 0x4,
	TYPE_WSTRING           = 0x5,
	TYPE_COLOR             = 0x6,
	TYPE_UINT64            = 0x7,
	TYPE_COMPILED_INT_BYTE = 0x8,
	TYPE_COMPILED_INT_0    = 0x9,
	TYPE_COMPILED_INT_1    = 0xA,
	TYPE_NUMTYPES          = 0xB,
};

class KeyValues
{
public:

	static void Init(void);
	KeyValues* FindKey(const char* pKeyName, bool bCreate);
	const char* GetName(void) const;
	int GetInt(const char* pKeyName, int nDefaultValue);
	void SetInt(const char* pKeyName, int iValue);
	void SetFloat(const char* keyName, float flValue);

	static void InitPlaylist(void);
	static bool LoadPlaylist(const char* szPlaylist);

	// Compiler makes it so m_Value shares the offset spot with m_flValue that why we cast it like this.
	MEMBER_AT_OFFSET(float, m_flValue, 0x18);
	MEMBER_AT_OFFSET(int, m_iValue, 0x18);

public:
	uint32_t m_iKeyName              : 24;         // 0x0000
	uint32_t m_iKeyNameCaseSensitive : 8;          // 0x0003
	char*            m_sValue;                     // 0x0008
	wchar_t*         m_wsValue;                    // 0x0010
	int              m_nValue;                     // 0x0018
private:
	char             gap1C[12];                    // 0x0020
public:
	char             m_iDataType;                  // 0x0028
	uint16_t         m_iKeyNameCaseSensitive2;     // 0x002A
	KeyValues*       m_pPeer;                      // 0x0030
	KeyValues*       m_pSub;                       // 0x0038
	KeyValues*       m_pChain;                     // 0x0040
};

///////////////////////////////////////////////////////////////////////////////
void CKeyValueSystem_Attach();
void CKeyValueSystem_Detach();

///////////////////////////////////////////////////////////////////////////////
extern KeyValues** g_pPlaylistKeyValues;

///////////////////////////////////////////////////////////////////////////////
class HKeyValues : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: KeyValues::Init                      : 0x" << std::hex << std::uppercase << p_KeyValues_Init.GetPtr()               << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: KeyValues::FindKey                   : 0x" << std::hex << std::uppercase << p_KeyValues_FindKey.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: KeyValues::LoadPlaylist              : 0x" << std::hex << std::uppercase << p_KeyValues_LoadPlaylist.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: KeyValues::GetCurrentPlaylist        : 0x" << std::hex << std::uppercase << p_KeyValues_GetCurrentPlaylist.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pPlaylistKeyValues                 : 0x" << std::hex << std::uppercase << g_pPlaylistKeyValues                    << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HKeyValues);
