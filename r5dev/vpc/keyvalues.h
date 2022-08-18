#pragma once
#include "mathlib/color.h"

#define MAKE_3_BYTES_FROM_1_AND_2( x1, x2 ) (( (( uint16_t )x2) << 8 ) | (uint8_t)(x1))
#define SPLIT_3_BYTES_INTO_1_AND_2( x1, x2, x3 ) do { x1 = (uint8)(x3); x2 = (uint16)( (x3) >> 8 ); } while( 0 )

extern vector<string> g_vAllPlaylists;
extern vector<string> g_vGameInfoPaths;

//---------------------------------------------------------------------------------
// Purpose: Forward declarations
//---------------------------------------------------------------------------------
class KeyValues;
class CFileSystem_Stdio;

/* ==== KEYVALUES ======================================================================================================================================================= */
inline CMemory p_KeyValues_FindKey;
inline auto KeyValues_FindKey = p_KeyValues_FindKey.RCast<void* (*)(KeyValues* thisptr, const char* pkeyName, bool bCreate)>();

inline CMemory p_KeyValues_LoadPlaylists;
inline auto KeyValues_LoadPlaylists = p_KeyValues_LoadPlaylists.RCast<bool (*)(const char* pszPlaylist)>();

inline CMemory p_KeyValues_ParsePlaylists;
inline auto KeyValues_ParsePlaylists = p_KeyValues_ParsePlaylists.RCast<bool (*)(const char* pszPlaylist)>();

inline CMemory p_KeyValues_GetCurrentPlaylist;
inline auto KeyValues_GetCurrentPlaylist = p_KeyValues_GetCurrentPlaylist.RCast<const char* (*)(void)>();

inline CMemory p_KeyValues_ReadKeyValuesFile;
inline auto KeyValues_ReadKeyValuesFile = p_KeyValues_ReadKeyValuesFile.RCast<KeyValues* (*)(CFileSystem_Stdio* pFileSystem, const char* pFileName)>();

enum KeyValuesTypes_t : char
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
enum MergeKeyValuesOp_t
{
	MERGE_KV_ALL,
	MERGE_KV_UPDATE,	// update values are copied into storage, adding new keys to storage or updating existing ones
	MERGE_KV_DELETE,	// update values specify keys that get deleted from storage
	MERGE_KV_BORROW,	// update values only update existing keys in storage, keys in update that do not exist in storage are discarded
};


//-----------------------------------------------------------------------------
// Purpose: Simple recursive data access class
//			Used in vgui for message parameters and resource files
//			Destructor deletes all child KeyValues nodes
//			Data is stored in key (string names) - (string/int/float)value pairs called nodes.
//
//	About KeyValues Text File Format:

//	It has 3 control characters '{', '}' and '"'. Names and values may be quoted or
//	not. The quote '"' character must not be used within name or values, only for
//	quoting whole tokens. You may use escape sequences wile parsing and add within a
//	quoted token a \" to add quotes within your name or token. When using Escape
//	Sequence the parser must now that by setting KeyValues::UsesEscapeSequences( true ),
//	which it's off by default. Non-quoted tokens ends with a whitespace, '{', '}' and '"'.
//	So you may use '{' and '}' within quoted tokens, but not for non-quoted tokens.
//  An open bracket '{' after a key name indicates a list of subkeys which is finished
//  with a closing bracket '}'. Subkeys use the same definitions recursively.
//  Whitespaces are space, return, newline and tabulator. Allowed Escape sequences
//	are \n, \t, \\, \n and \". The number character '#' is used for macro purposes 
//	(eg #include), don't use it as first character in key names.
//-----------------------------------------------------------------------------
class KeyValues
{
public:
	// Constructors/destructors
	KeyValues(const char* pszSetName);
	KeyValues(const char* pszSetName, const char* pszFirstKey, const char* pszFirstValue);
	KeyValues(const char* pszSetName, const char* pszFirstKey, const wchar_t* pwszFirstValue);
	KeyValues(const char* pszSetName, const char* pszFirstKey, int iFirstValue);
	KeyValues(const char* pszSetName, const char* pszFirstKey, const char* pszFirstValue, const char* pszSecondKey, const char* pszSecondValue);
	KeyValues(const char* pszSetName, const char* pszFirstKey, int iFirstValue, const char* pszSecondKey, int iSecondValue);
	~KeyValues(void);

	void Init(void);
	void Clear(void);
	void DeleteThis(void);
	void RemoveEverything();

	KeyValues* FindKey(const char* pKeyName, bool bCreate = false);
	KeyValues* FindLastSubKey(void) const;

	void AddSubKey(KeyValues* pSubkey);
	void RemoveSubKey(KeyValues* pSubKey);
	void InsertSubKey(int nIndex, KeyValues* pSubKey);
	bool ContainsSubKey(KeyValues* pSubKey);
	void SwapSubKey(KeyValues* pExistingSubkey, KeyValues* pNewSubKey);
	void ElideSubKey(KeyValues* pSubKey);

	// Data access
	bool IsEmpty(const char* pszKeyName);
	KeyValues* GetFirstTrueSubKey(void) const;
	KeyValues* GetNextTrueSubKey(void) const;
	KeyValues* GetFirstValue(void) const;
	KeyValues* GetNextValue(void) const;
	KeyValues* GetFirstSubKey() const;
	KeyValues* GetNextKey() const;
	const char* GetName(void) const;
	int GetInt(const char* pszKeyName, int iDefaultValue);
	uint64_t GetUint64(const char* pszKeyName, uint64_t nDefaultValue);
	void* GetPtr(const char* pszKeyName, void* pDefaultValue);
	float GetFloat(const char* pszKeyName, float flDefaultValue);
	const char* GetString(const char* pszKeyName = nullptr, const char* pszDefaultValue = "");
	const wchar_t* GetWString(const char* pszKeyName = nullptr, const wchar_t* pwszDefaultValue = L"");
	Color GetColor(const char* pszKeyName, const Color& defaultColor);
	KeyValuesTypes_t GetDataType(const char* pszKeyName);
	KeyValuesTypes_t GetDataType(void) const;

	// Key writing
	void SetInt(const char* pszKeyName, int iValue);
	void SetUint64(const char* pszKeyName, uint64_t nValue);
	void SetPtr(const char* pszKeyName, void* pValue);
	void SetNextKey(KeyValues* pDat);
	void SetName(const char* pszName);
	void SetString(const char* pszKeyName, const char* pszValue);
	void SetWString(const char* pszKeyName, const wchar_t* pwszValue);
	void SetStringValue(char const* pszValue);
	void SetColor(const char* pszKeyName, Color color);
	void SetFloat(const char* pszKeyName, float flValue);

	void RecursiveCopyKeyValues(KeyValues& src);
	void CopySubkeys(KeyValues* pParent) const;
	KeyValues* MakeCopy(void) const;

	// Initialization
	static void InitPlaylists(void);
	static void InitFileSystem(void);
	static bool LoadPlaylists(const char* szPlaylist);
	static bool ParsePlaylists(const char* szPlaylist);
	static KeyValues* ReadKeyValuesFile(CFileSystem_Stdio* pFileSystem, const char* pFileName);

public:
	uint32_t m_iKeyName               : 24;        // 0x0000
	uint32_t m_iKeyNameCaseSensitive1 : 8;         // 0x0003
	char*            m_sValue;                     // 0x0008
	wchar_t*         m_wsValue;                    // 0x0010
	union                                          // 0x0018
	{
		int           m_iValue;
		float         m_flValue;
		void*         m_pValue;
		unsigned char m_Color[4];
	};
	char             m_szShortName[8];             // 0x0020
	char             m_iDataType;                  // 0x0028
	char             m_bHasEscapeSequences;        // 0x0029
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
class VKeyValues : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: KeyValues::FindKey                   : {:#18x} |\n", p_KeyValues_FindKey.GetPtr());
		spdlog::debug("| FUN: KeyValues::LoadPlaylists             : {:#18x} |\n", p_KeyValues_LoadPlaylists.GetPtr());
		spdlog::debug("| FUN: KeyValues::ParsePlaylists            : {:#18x} |\n", p_KeyValues_ParsePlaylists.GetPtr());
		spdlog::debug("| FUN: KeyValues::GetCurrentPlaylist        : {:#18x} |\n", p_KeyValues_GetCurrentPlaylist.GetPtr());
		spdlog::debug("| FUN: KeyValues::ReadKeyValuesFile         : {:#18x} |\n", p_KeyValues_ReadKeyValuesFile.GetPtr());
		spdlog::debug("| VAR: g_pPlaylistKeyValues                 : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pPlaylistKeyValues));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_KeyValues_FindKey            = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x10\x48\x89\x6C\x24\x18\x48\x89\x74\x24\x20\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x20\x01\x00\x00\x45"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		p_KeyValues_GetCurrentPlaylist = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x75\x08\x48\x8D\x05\x00\x00\x00\x00"), "xxx????xxxxxxxx????");
		p_KeyValues_ReadKeyValuesFile  = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x57\x48\x8D\x6C\x24\x00"), "xxxx?xxxxxxxxxxx?");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_KeyValues_FindKey            = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x57\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x45"), "xxxxxxxx????x");
		p_KeyValues_GetCurrentPlaylist = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x08\x48\x8D\x05\x00\x00\x00\x00\xC3\x0F\xB7\x50\x2A"), "xxx????xxxxxxxx????xxxxx");
		p_KeyValues_ReadKeyValuesFile  = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x53\x57\x41\x54\x48\x8D\x68\xA1"), "xxxxxxxxxxxx");
#endif
		p_KeyValues_LoadPlaylists        = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\x48\x83\xEC\x40\x48\x8B\xF1"), "xxxx?xxxx?xxxxxxxxxxx");
		p_KeyValues_ParsePlaylists       = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x74\x0C"), "x????xx?????xx").FollowNearCallSelf();

		KeyValues_FindKey            = p_KeyValues_FindKey.RCast<void* (*)(KeyValues*, const char*, bool)>();                  /*40 56 57 41 57 48 81 EC 30 01 00 00 45 0F B6 F8*/
		KeyValues_LoadPlaylists      = p_KeyValues_ParsePlaylists.RCast<bool (*)(const char*)>();                              /*48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 40 48 8B F1*/
		KeyValues_ParsePlaylists     = p_KeyValues_ParsePlaylists.RCast<bool (*)(const char*)>();                              /*E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 0C*/
		KeyValues_GetCurrentPlaylist = p_KeyValues_GetCurrentPlaylist.RCast<const char* (*)(void)>();                          /*48 8B 05 ?? ?? ?? ?? 48 85 C0 75 08 48 8D 05 ?? ?? ?? ?? C3 0F B7 50 2A*/
		KeyValues_ReadKeyValuesFile  = p_KeyValues_ReadKeyValuesFile.RCast<KeyValues* (*)(CFileSystem_Stdio*, const char*)>(); /*48 8B C4 55 53 57 41 54 48 8D 68 A1*/
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pPlaylistKeyValues = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x8B\xC4\x53\x57\x41\x56\x48\x81\xEC\x20"), 
			"xxxxxxxxxxx").FindPatternSelf("48 8B 2D", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<KeyValues**>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pPlaylistKeyValues = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\xB4"), 
			"xxxxxxxxxxxxxxxxxxxxxxxxx").FindPatternSelf("48 8B 0D", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<KeyValues**>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VKeyValues);
