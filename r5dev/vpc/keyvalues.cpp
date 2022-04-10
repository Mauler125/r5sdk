//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "vpc/keyvalues.h"
#include "vstdlib/keyvaluessystem.h"
#include "rtech/stryder/stryder.h"
#include "engine/sys_dll2.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void KeyValues::Init(void)
{
	std::thread t1(KeyValues::InitPlaylist); // Start thread to grab playlists.
	t1.detach(); // Detach thread from current one.
}

//-----------------------------------------------------------------------------
// Purpose: Find a keyValue, create it if it is not found.
//			Set bCreate to true to create the key if it doesn't already exist 
//			(which ensures a valid pointer will be returned)
// Input  : *pKeyName - 
//			bCreate - 
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::FindKey(const char* keyName, bool bCreate)
{
	static auto func = reinterpret_cast<KeyValues * (__thiscall*)(KeyValues*, const char*, bool)>(KeyValues_FindKey);
	return func(this, keyName, bCreate);
}

//-----------------------------------------------------------------------------
// Purpose: Get the name of the current key section
// Output : const char*
//-----------------------------------------------------------------------------
const char* KeyValues::GetName(void) const
{
	return g_pKeyValuesSystem->GetStringForSymbol(MAKE_3_BYTES_FROM_1_AND_2(m_iKeyNameCaseSensitive, m_iKeyNameCaseSensitive2));
}

//-----------------------------------------------------------------------------
// Purpose: Get the integer value of a keyName. Default value is returned
//			if the keyName can't be found.
// Input  : *pKeyName - 
//			nDefaultValue - 
// Output : int
//-----------------------------------------------------------------------------
int KeyValues::GetInt(const char* pKeyName, int nDefaultValue)
{
	KeyValues* dat = FindKey(pKeyName, false);

	if (!dat)
		return nDefaultValue;

	switch (dat->m_iDataType)
	{
	case TYPE_STRING:
		return atoi(dat->m_sValue);
	case TYPE_FLOAT:
		return static_cast<int>(m_flValue());
	case TYPE_WSTRING:
		return _wtoi(dat->m_wsValue);
	case TYPE_UINT64:
		return 0;
	default:
		return dat->m_iValue();
	}

	return nDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Set the integer value of a keyName. 
// Input  : *pKeyName - 
//			iValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetInt(const char* pKeyName, int iValue)
{
	KeyValues* dat = FindKey(pKeyName, true);
	if (dat)
	{
		dat->m_iValue() = iValue;
		dat->m_iDataType = TYPE_INT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the float value of a keyName. 
// Input  : *pKeyName - 
//			flValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetFloat(const char* pKeyName, float flValue)
{
	KeyValues* dat = FindKey(pKeyName, true);
	if (dat)
	{
		dat->m_flValue() = flValue;
		dat->m_iDataType = TYPE_FLOAT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the playlist
//-----------------------------------------------------------------------------
void KeyValues::InitPlaylist(void)
{
	while (true)
	{
		if ((*g_pPlaylistKeyValues))
		{
			KeyValues* playlists = (*g_pPlaylistKeyValues)->FindKey("Playlists", false);
			if (playlists)
			{
				g_szAllPlaylists.clear();
				for (KeyValues* dat = playlists->m_pSub; dat != nullptr; dat = dat->m_pPeer) // Parse through all sub keys.
				{
					g_szAllPlaylists.push_back(dat->GetName()); // Get all playlist names.
				}

				break; // Break if playlist got filled.
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads the playlist
// Input  : *szPlaylist - 
//-----------------------------------------------------------------------------
bool KeyValues::LoadPlaylist(const char* szPlaylist)
{
	memset(g_pMapVPKCache, '\0', 0x40); // Clear VPK cache to prevent crash while loading playlist.

	CHAR sPlaylistPath[] = "\x77\x27\x35\x2b\x2c\x6c\x2b\x2c\x2b";
	PCHAR curr = sPlaylistPath;
	while (*curr)
	{
		*curr ^= 'B';
		++curr;
	}

	if (FileExists(sPlaylistPath))
	{
		std::uint8_t verifyPlaylistIntegrity[] = // Very hacky way for alternative inline assembly for x64..
		{
			0x48, 0x8B, 0x45, 0x58,       // mov rcx, playlist
			0xC7, 0x00, 0x00, 0x00, 0x00, // test playlist, playlist
			0x00
		};
		void* verifyPlaylistIntegrityFn = nullptr;
		VirtualAlloc(verifyPlaylistIntegrity, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		memcpy(&verifyPlaylistIntegrityFn, (const void*)verifyPlaylistIntegrity, 9);
		reinterpret_cast<void(*)()>(verifyPlaylistIntegrityFn)();
	}

	return KeyValues_LoadPlaylist(szPlaylist); // Parse playlist.
}

///////////////////////////////////////////////////////////////////////////////
void CKeyValueSystem_Attach()
{
	DetourAttach((LPVOID*)&KeyValues_LoadPlaylist, &KeyValues::LoadPlaylist);
}

void CKeyValueSystem_Detach()
{
	DetourDetach((LPVOID*)&KeyValues_LoadPlaylist, &KeyValues::LoadPlaylist);
}

///////////////////////////////////////////////////////////////////////////////
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline KeyValues** g_pPlaylistKeyValues = reinterpret_cast<KeyValues**>(p_Stryder_StitchRequest.FindPatternSelf("48 8B 2D", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr()); // Get the KeyValue for the playlist file.
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline KeyValues** g_pPlaylistKeyValues = reinterpret_cast<KeyValues**>(p_Stryder_StitchRequest.FindPatternSelf("48 8B 0D", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr()); // Get the KeyValue for the playlist file.
#endif
vector<string> g_szAllPlaylists = { "<<null>>" };