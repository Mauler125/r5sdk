//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/memstd.h"
#include "tier1/strtools.h"
#include "vpc/keyvalues.h"
#include "vpc/kvleaktrace.h"
#include "vstdlib/keyvaluessystem.h"
#include "filesystem/filesystem.h"
#include "mathlib/color.h"
#include "rtech/stryder/stryder.h"
#include "engine/sys_dll2.h"
#include "engine/cmodel_bsp.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pszSetName - 
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char* pszSetName)
{
	TRACK_KV_ADD(this, pszSetName);

	Init();
	SetName(pszSetName);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pszSetName - 
//			*pszFirstKey - 
//			*pszFirstValue - 
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char* pszSsetName, const char* pszFirstKey, const char* pszFirstValue)
{
	TRACK_KV_ADD(this, pszSsetName);

	Init();
	SetName(pszSsetName);
	SetString(pszFirstKey, pszFirstValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pszSetName - 
//			*pszFirstKey - 
//			*pwszFirstValue - 
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char* pszSetName, const char* pszFirstKey, const wchar_t* pwszFirstValue)
{
	TRACK_KV_ADD(this, pszSetName);

	Init();
	SetName(pszSetName);
	SetWString(pszFirstKey, pwszFirstValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pszSetName - 
//			*pszFirstKey - 
//			iFirstValue - 
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char* pszSetName, const char* pszFirstKey, int iFirstValue)
{
	TRACK_KV_ADD(this, pszSetName);

	Init();
	SetName(pszSetName);
	SetInt(pszFirstKey, iFirstValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pszSetName - 
//			*pszFirstKey - 
//			*pszFirstValue - 
//			*pszSecondKey - 
//			*pszSecondValue - 
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char* pszSetName, const char* pszFirstKey, const char* pszFirstValue, const char* pszSecondKey, const char* pszSecondValue)
{
	TRACK_KV_ADD(this, pszSetName);

	Init();
	SetName(pszSetName);
	SetString(pszFirstKey, pszFirstValue);
	SetString(pszSecondKey, pszSecondValue);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *pszSetName - 
//			*pszFirstKey - 
//			iFirstValue - 
//			*pszSecondKey - 
//			iSecondValue - 
//-----------------------------------------------------------------------------
KeyValues::KeyValues(const char* pszSetName, const char* pszFirstKey, int iFirstValue, const char* pszSecondKey, int iSecondValue)
{
	TRACK_KV_ADD(this, pszSetName);

	Init();
	SetName(pszSetName);
	SetInt(pszFirstKey, iFirstValue);
	SetInt(pszSecondKey, iSecondValue);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
KeyValues::~KeyValues(void)
{
	TRACK_KV_REMOVE(this);

	RemoveEverything();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize member variables
//-----------------------------------------------------------------------------
void KeyValues::Init(void)
{
	m_iKeyName = 0;
	m_iKeyNameCaseSensitive1 = 0;
	m_iKeyNameCaseSensitive2 = 0;
	m_iDataType = TYPE_NONE;

	m_pSub = nullptr;
	m_pPeer = nullptr;
	m_pChain = nullptr;

	m_sValue = nullptr;
	m_wsValue = nullptr;
	m_pValue = nullptr;

	m_bHasEscapeSequences = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Clear out all subkeys, and the current value
//-----------------------------------------------------------------------------
void KeyValues::Clear(void)
{
	delete m_pSub;
	m_pSub = nullptr;
	m_iDataType = TYPE_NONE;
}

//-----------------------------------------------------------------------------
// for backwards compat - we used to need this to force the free to run from the same DLL
// as the alloc
//-----------------------------------------------------------------------------
void KeyValues::DeleteThis(void)
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: remove everything
//-----------------------------------------------------------------------------
void KeyValues::RemoveEverything(void)
{
	KeyValues* dat;
	KeyValues* datNext = nullptr;
	for (dat = m_pSub; dat != nullptr; dat = datNext)
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = nullptr;
		delete dat;
	}

	for (dat = m_pPeer; dat && dat != this; dat = datNext)
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = nullptr;
		delete dat;
	}

	delete[] m_sValue;
	m_sValue = nullptr;
	delete[] m_wsValue;
	m_wsValue = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Find a keyValue, create it if it is not found.
//			Set bCreate to true to create the key if it doesn't already exist 
//			(which ensures a valid pointer will be returned)
// Input  : *pszKeyName - 
//			bCreate - 
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::FindKey(const char* pszKeyName, bool bCreate)
{
	// Validate NULL == this early out
	if (!this)
	{
		// Undefined behavior. Could blow up on a new platform. Don't do it.
		DevWarning(eDLL_T::COMMON, "KeyValues::FindKey called on NULL pointer!"); 
		Assert(!bCreate);
		return nullptr;
	}

	// return the current key if a NULL subkey is asked for
	if (!pszKeyName || !pszKeyName[0])
		return this;

	// look for '/' characters delimiting sub fields
	char szBuf[256];
	const char* subStr = strchr(pszKeyName, '/');
	const char* searchStr = pszKeyName;

	// pull out the substring if it exists
	if (subStr)
	{
		ptrdiff_t size = subStr - pszKeyName;
		memcpy(szBuf, pszKeyName, size);
		szBuf[size] = '\0';
		searchStr = szBuf;
	}

	// lookup the symbol for the search string,
	// we do not need the case-sensitive symbol at this time
	// because if the key is found, then it will be found by case-insensitive lookup
	// if the key is not found and needs to be created we will pass the actual searchStr
	// and have the new KeyValues constructor get/create the case-sensitive symbol
	HKeySymbol iSearchStr = KeyValuesSystem()->GetSymbolForString(searchStr, bCreate);
	if (iSearchStr == INVALID_KEY_SYMBOL)
	{
		// not found, couldn't possibly be in key value list
		return nullptr;
	}

	KeyValues* lastItem = nullptr;
	KeyValues* dat;
	// find the searchStr in the current peer list
	for (dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		lastItem = dat;	// record the last item looked at (for if we need to append to the end of the list)

		// symbol compare
		if (dat->m_iKeyName == (uint32)iSearchStr)
		{
			break;
		}
	}

	if (!dat && m_pChain)
	{
		dat = m_pChain->FindKey(pszKeyName, false);
	}

	// make sure a key was found
	if (!dat)
	{
		if (bCreate)
		{
			// we need to create a new key
			dat = new KeyValues(searchStr);
			Assert(dat != nullptr);

			// insert new key at end of list
			if (lastItem)
			{
				lastItem->m_pPeer = dat;
			}
			else
			{
				m_pSub = dat;
			}
			dat->m_pPeer = nullptr;

			// a key graduates to be a submsg as soon as it's m_pSub is set
			// this should be the only place m_pSub is set
			m_iDataType = TYPE_NONE;
		}
		else
		{
			return nullptr;
		}
	}

	// if we've still got a subStr we need to keep looking deeper in the tree
	if (subStr)
	{
		// recursively chain down through the paths in the string
		return dat->FindKey(subStr + 1, bCreate);
	}

	return dat;
}

//-----------------------------------------------------------------------------
// Purpose: Locate last child.  Returns NULL if we have no children
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::FindLastSubKey(void) const
{
	// No children?
	if (m_pSub == nullptr)
		return nullptr;

	// Scan for the last one
	KeyValues* pLastChild = m_pSub;
	while (pLastChild->m_pPeer)
		pLastChild = pLastChild->m_pPeer;
	return pLastChild;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a subkey. Make sure the subkey isn't a child of some other keyvalues
// Input  : *pSubKey - 
//-----------------------------------------------------------------------------
void KeyValues::AddSubKey(KeyValues* pSubkey)
{
	// Make sure the subkey isn't a child of some other keyvalues
	Assert(pSubkey != nullptr);
	Assert(pSubkey->m_pPeer == nullptr);

	// add into subkey list
	if (m_pSub == nullptr)
	{
		m_pSub = pSubkey;
	}
	else
	{
		KeyValues* pTempDat = m_pSub;
		while (pTempDat->GetNextKey() != nullptr)
		{
			pTempDat = pTempDat->GetNextKey();
		}

		pTempDat->SetNextKey(pSubkey);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove a subkey from the list
// Input  : *pSubKey - 
//-----------------------------------------------------------------------------
void KeyValues::RemoveSubKey(KeyValues* pSubKey)
{
	if (!pSubKey)
		return;

	// check the list pointer
	if (m_pSub == pSubKey)
	{
		m_pSub = pSubKey->m_pPeer;
	}
	else
	{
		// look through the list
		KeyValues* kv = m_pSub;
		while (kv->m_pPeer)
		{
			if (kv->m_pPeer == pSubKey)
			{
				kv->m_pPeer = pSubKey->m_pPeer;
				break;
			}

			kv = kv->m_pPeer;
		}
	}

	pSubKey->m_pPeer = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Insert a subkey at index
// Input  : nIndex - 
//			*pSubKey - 
//-----------------------------------------------------------------------------
void KeyValues::InsertSubKey(int nIndex, KeyValues* pSubKey)
{
	// Sub key must be valid and not part of another chain
	Assert(pSubKey && pSubKey->m_pPeer == nullptr);

	if (nIndex == 0)
	{
		pSubKey->m_pPeer = m_pSub;
		m_pSub = pSubKey;
		return;
	}
	else
	{
		int nCurrentIndex = 0;
		for (KeyValues* pIter = GetFirstSubKey(); pIter != nullptr; pIter = pIter->GetNextKey())
		{
			++nCurrentIndex;
			if (nCurrentIndex == nIndex)
			{
				pSubKey->m_pPeer = pIter->m_pPeer;
				pIter->m_pPeer = pSubKey;
				return;
			}
		}
		// Index is out of range if we get here
		Assert(0);
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if key contains a subkey
// Input  : *pSubKey - 
// Output : true if contains, false otherwise
//-----------------------------------------------------------------------------
bool KeyValues::ContainsSubKey(KeyValues* pSubKey)
{
	for (KeyValues* pIter = GetFirstSubKey(); pIter != nullptr; pIter = pIter->GetNextKey())
	{
		if (pSubKey == pIter)
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Swaps existing subkey with another
// Input  : *pExistingSubkey - 
//			*pNewSubKey - 
//-----------------------------------------------------------------------------
void KeyValues::SwapSubKey(KeyValues* pExistingSubkey, KeyValues* pNewSubKey)
{
	Assert(pExistingSubkey != nullptr && pNewSubKey != nullptr);

	// Make sure the new sub key isn't a child of some other keyvalues
	Assert(pNewSubKey->m_pPeer == nullptr);

	// Check the list pointer
	if (m_pSub == pExistingSubkey)
	{
		pNewSubKey->m_pPeer = pExistingSubkey->m_pPeer;
		pExistingSubkey->m_pPeer = nullptr;
		m_pSub = pNewSubKey;
	}
	else
	{
		// Look through the list
		KeyValues* kv = m_pSub;
		while (kv->m_pPeer)
		{
			if (kv->m_pPeer == pExistingSubkey)
			{
				pNewSubKey->m_pPeer = pExistingSubkey->m_pPeer;
				pExistingSubkey->m_pPeer = nullptr;
				kv->m_pPeer = pNewSubKey;
				break;
			}

			kv = kv->m_pPeer;
		}
		// Existing sub key should always be found, otherwise it's a bug in the calling code.
		Assert(kv->m_pPeer != nullptr);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Elides subkey
// Input  : *pSubKey - 
//-----------------------------------------------------------------------------
void KeyValues::ElideSubKey(KeyValues* pSubKey)
{
	// This pointer's "next" pointer needs to be fixed up when we elide the key
	KeyValues** ppPointerToFix = &m_pSub;
	for (KeyValues* pKeyIter = m_pSub; pKeyIter != nullptr; ppPointerToFix = &pKeyIter->m_pPeer, pKeyIter = pKeyIter->GetNextKey())
	{
		if (pKeyIter == pSubKey)
		{
			if (pSubKey->m_pSub == nullptr)
			{
				// No children, simply remove the key
				*ppPointerToFix = pSubKey->m_pPeer;
				delete pSubKey;
			}
			else
			{
				*ppPointerToFix = pSubKey->m_pSub;
				// Attach the remainder of this chain to the last child of pSubKey
				KeyValues* pChildIter = pSubKey->m_pSub;
				while (pChildIter->m_pPeer != nullptr)
				{
					pChildIter = pChildIter->m_pPeer;
				}
				// Now points to the last child of pSubKey
				pChildIter->m_pPeer = pSubKey->m_pPeer;
				// Detach the node to be elided
				pSubKey->m_pSub = nullptr;
				pSubKey->m_pPeer = nullptr;
				delete pSubKey;
			}
			return;
		}
	}
	// Key not found; that's caller error.
	Assert(0);
}

//-----------------------------------------------------------------------------
// Purpose: Check if a keyName has no value assigned to it.
// Input  : *pszKeyName - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool KeyValues::IsEmpty(const char* pszKeyName)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (!pKey)
		return true;

	if (pKey->m_iDataType == TYPE_NONE && pKey->m_pSub == nullptr)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: gets the first true sub key
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::GetFirstTrueSubKey(void) const
{
	Assert(this, "Member function called on NULL KeyValues");
	KeyValues* pRet = this ? m_pSub : nullptr;
	while (pRet && pRet->m_iDataType != TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: gets the next true sub key
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::GetNextTrueSubKey(void) const
{
	Assert(this, "Member function called on NULL KeyValues");
	KeyValues* pRet = this ? m_pPeer : nullptr;
	while (pRet && pRet->m_iDataType != TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: gets the first value
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::GetFirstValue(void) const
{
	Assert(this, "Member function called on NULL KeyValues");
	KeyValues* pRet = this ? m_pSub : nullptr;
	while (pRet && pRet->m_iDataType == TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: gets the next value
// Output : *KeyValues
//-----------------------------------------------------------------------------
KeyValues* KeyValues::GetNextValue(void) const
{
	Assert(this, "Member function called on NULL KeyValues");
	KeyValues* pRet = this ? m_pPeer : nullptr;
	while (pRet && pRet->m_iDataType == TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: Return the first subkey in the list
//-----------------------------------------------------------------------------
KeyValues* KeyValues::GetFirstSubKey() const
{
	Assert(this, "Member function called on NULL KeyValues");
	return this ? m_pSub : nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Return the next subkey
//-----------------------------------------------------------------------------
KeyValues* KeyValues::GetNextKey() const
{
	Assert(this, "Member function called on NULL KeyValues");
	return this ? m_pPeer : nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Get the name of the current key section
// Output : const char*
//-----------------------------------------------------------------------------
const char* KeyValues::GetName(void) const
{
	return KeyValuesSystem()->GetStringForSymbol(MAKE_3_BYTES_FROM_1_AND_2(m_iKeyNameCaseSensitive1, m_iKeyNameCaseSensitive2));
}

//-----------------------------------------------------------------------------
// Purpose: Get the integer value of a keyName. Default value is returned
//			if the keyName can't be found.
// Input  : *pszKeyName - 
//			nDefaultValue - 
// Output : int
//-----------------------------------------------------------------------------
int KeyValues::GetInt(const char* pszKeyName, int iDefaultValue)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		switch (pKey->m_iDataType)
		{
		case TYPE_STRING:
			return atoi(pKey->m_sValue);
		case TYPE_WSTRING:
			return _wtoi(pKey->m_wsValue);
		case TYPE_FLOAT:
			return static_cast<int>(pKey->m_flValue);
		case TYPE_UINT64:
			// can't convert, since it would lose data
			Assert(0);
			return 0;
		case TYPE_INT:
		case TYPE_PTR:
		default:
			return pKey->m_iValue;
		};
	}
	return iDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the integer value of a keyName. Default value is returned
//			if the keyName can't be found.
// Input  : *pszKeyName - 
//			nDefaultValue - 
// Output : uint64_t
//-----------------------------------------------------------------------------
uint64_t KeyValues::GetUint64(const char* pszKeyName, uint64_t nDefaultValue)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		switch (pKey->m_iDataType)
		{
		case TYPE_STRING:
		{
			uint64_t uiResult = 0ull;
			sscanf(pKey->m_sValue, "%lld", &uiResult);
			return uiResult;
		}
		case TYPE_WSTRING:
		{
			uint64_t uiResult = 0ull;
			swscanf(pKey->m_wsValue, L"%lld", &uiResult);
			return uiResult;
		}
		case TYPE_FLOAT:
			return static_cast<int>(pKey->m_flValue);
		case TYPE_UINT64:
			return *reinterpret_cast<uint64_t*>(pKey->m_sValue);
		case TYPE_PTR:
			return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pKey->m_pValue));
		case TYPE_INT:
		default:
			return pKey->m_iValue;
		};
	}
	return nDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the pointer value of a keyName. Default value is returned
//			if the keyName can't be found.
// Input  : *pszKeyName - 
//			pDefaultValue - 
// Output : void*
//-----------------------------------------------------------------------------
void* KeyValues::GetPtr(const char* pszKeyName, void* pDefaultValue)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		switch (pKey->m_iDataType)
		{
		case TYPE_PTR:
			return pKey->m_pValue;

		case TYPE_WSTRING:
		case TYPE_STRING:
		case TYPE_FLOAT:
		case TYPE_INT:
		case TYPE_UINT64:
		default:
			return nullptr;
		};
	}
	return pDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the float value of a keyName. Default value is returned
//			if the keyName can't be found.
// Input  : *pszKeyName - 
//			flDefaultValue - 
// Output : float
//-----------------------------------------------------------------------------
float KeyValues::GetFloat(const char* pszKeyName, float flDefaultValue)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		switch (pKey->m_iDataType)
		{
		case TYPE_STRING:
			return static_cast<float>(atof(pKey->m_sValue));
		case TYPE_WSTRING:
			return static_cast<float>(_wtof(pKey->m_wsValue));		// no wtof
		case TYPE_FLOAT:
			return pKey->m_flValue;
		case TYPE_INT:
			return static_cast<float>(pKey->m_iValue);
		case TYPE_UINT64:
			return static_cast<float>((*(reinterpret_cast<uint64*>(pKey->m_sValue))));
		case TYPE_PTR:
		default:
			return 0.0f;
		};
	}
	return flDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the string pointer of a keyName. Default value is returned
//			if the keyName can't be found.
// // Input  : *pszKeyName - 
//			pszDefaultValue - 
// Output : const char*
//-----------------------------------------------------------------------------
const char* KeyValues::GetString(const char* pszKeyName, const char* pszDefaultValue)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		// convert the data to string form then return it
		char buf[64];
		switch (pKey->m_iDataType)
		{
		case TYPE_FLOAT:
			snprintf(buf, sizeof(buf), "%f", pKey->m_flValue);
			SetString(pszKeyName, buf);
			break;
		case TYPE_PTR:
			snprintf(buf, sizeof(buf), "%lld", CastPtrToInt64(pKey->m_pValue));
			SetString(pszKeyName, buf);
			break;
		case TYPE_INT:
			snprintf(buf, sizeof(buf), "%d", pKey->m_iValue);
			SetString(pszKeyName, buf);
			break;
		case TYPE_UINT64:
			snprintf(buf, sizeof(buf), "%lld", *(reinterpret_cast<uint64*>(pKey->m_sValue)));
			SetString(pszKeyName, buf);
			break;
		case TYPE_COLOR:
			snprintf(buf, sizeof(buf), "%d %d %d %d", pKey->m_Color[0], pKey->m_Color[1], pKey->m_Color[2], pKey->m_Color[3]);
			SetString(pszKeyName, buf);
			break;

		case TYPE_WSTRING:
		{
			// convert the string to char *, set it for future use, and return it
			char wideBuf[512];
			int result = V_UnicodeToUTF8(pKey->m_wsValue, wideBuf, 512);
			if (result)
			{
				// note: this will copy wideBuf
				SetString(pszKeyName, wideBuf);
			}
			else
			{
				return pszDefaultValue;
			}
			break;
		}
		case TYPE_STRING:
			break;
		default:
			return pszDefaultValue;
		};

		return pKey->m_sValue;
	}
	return pszDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the wide string pointer of a keyName. Default value is returned
//			if the keyName can't be found.
// // Input  : *pszKeyName - 
//			pwszDefaultValue - 
// Output : const wchar_t*
//-----------------------------------------------------------------------------
const wchar_t* KeyValues::GetWString(const char* pszKeyName, const wchar_t* pwszDefaultValue)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		wchar_t wbuf[64];
		switch (pKey->m_iDataType)
		{
		case TYPE_FLOAT:
			swprintf(wbuf, Q_ARRAYSIZE(wbuf), L"%f", pKey->m_flValue);
			SetWString(pszKeyName, wbuf);
			break;
		case TYPE_PTR:
			swprintf(wbuf, Q_ARRAYSIZE(wbuf), L"%lld", static_cast<int64_t>(reinterpret_cast<size_t>(pKey->m_pValue)));
			SetWString(pszKeyName, wbuf);
			break;
		case TYPE_INT:
			swprintf(wbuf, Q_ARRAYSIZE(wbuf), L"%d", pKey->m_iValue);
			SetWString(pszKeyName, wbuf);
			break;
		case TYPE_UINT64:
		{
			swprintf(wbuf, Q_ARRAYSIZE(wbuf), L"%lld", *(reinterpret_cast<uint64_t*>(pKey->m_sValue)));
			SetWString(pszKeyName, wbuf);
		}
		break;
		case TYPE_COLOR:
			swprintf(wbuf, Q_ARRAYSIZE(wbuf), L"%d %d %d %d", pKey->m_Color[0], pKey->m_Color[1], pKey->m_Color[2], pKey->m_Color[3]);
			SetWString(pszKeyName, wbuf);
			break;

		case TYPE_WSTRING:
			break;
		case TYPE_STRING:
		{
			size_t bufSize = strlen(pKey->m_sValue) + 1;
			wchar_t* pWBuf = new wchar_t[bufSize];
			int result = V_UTF8ToUnicode(pKey->m_sValue, pWBuf, int(bufSize * sizeof(wchar_t)));
			if (result >= 0) // may be a zero length string
			{
				SetWString(pszKeyName, pWBuf);
			}
			else
			{
				delete[] pWBuf;
				return pwszDefaultValue;
			}
			delete[] pWBuf;
			break;
		}
		default:
			return pwszDefaultValue;
		};

		return reinterpret_cast<const wchar_t*>(pKey->m_wsValue);
	}
	return pwszDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a color
// Input  : *pszKeyName - 
//			&defaultColor - 
// Output : Color
//-----------------------------------------------------------------------------
Color KeyValues::GetColor(const char* pszKeyName, const Color& defaultColor)
{
	Color color = defaultColor;
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
	{
		if (pKey->m_iDataType == TYPE_COLOR)
		{
			color[0] = pKey->m_Color[0];
			color[1] = pKey->m_Color[1];
			color[2] = pKey->m_Color[2];
			color[3] = pKey->m_Color[3];
		}
		else if (pKey->m_iDataType == TYPE_FLOAT)
		{
			color[0] = static_cast<unsigned char>(pKey->m_flValue);
		}
		else if (pKey->m_iDataType == TYPE_INT)
		{
			color[0] = static_cast<unsigned char>(pKey->m_iValue);
		}
		else if (pKey->m_iDataType == TYPE_STRING)
		{
			// parse the colors out of the string
			float a = 0, b = 0, c = 0, d = 0;
			sscanf(pKey->m_sValue, "%f %f %f %f", &a, &b, &c, &d);
			color[0] = static_cast<unsigned char>(a);
			color[1] = static_cast<unsigned char>(b);
			color[2] = static_cast<unsigned char>(c);
			color[3] = static_cast<unsigned char>(d);
		}
	}
	return color;
}

//-----------------------------------------------------------------------------
// Purpose: Get the data type of the value stored in a keyName
// Input  : *pszKeyName - 
//-----------------------------------------------------------------------------
KeyValuesTypes_t KeyValues::GetDataType(const char* pszKeyName)
{
	KeyValues* pKey = FindKey(pszKeyName, false);
	if (pKey)
		return static_cast<KeyValuesTypes_t>(pKey->m_iDataType);

	return TYPE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Get the data type of the value stored in this keyName
//-----------------------------------------------------------------------------
KeyValuesTypes_t KeyValues::GetDataType(void) const
{
	return static_cast<KeyValuesTypes_t>(m_iDataType);
}

//-----------------------------------------------------------------------------
// Purpose: Set the integer value of a keyName. 
// Input  : *pszKeyName - 
//			iValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetInt(const char* pszKeyName, int iValue)
{
	KeyValues* pKey = FindKey(pszKeyName, true);
	if (pKey)
	{
		pKey->m_iValue = iValue;
		pKey->m_iDataType = TYPE_INT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the integer value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetUint64(const char* pszKeyName, uint64_t nValue)
{
	KeyValues* pKey = FindKey(pszKeyName, true);

	if (pKey)
	{
		// delete the old value
		delete[] pKey->m_sValue;
		// make sure we're not storing the WSTRING  - as we're converting over to STRING
		delete[] pKey->m_wsValue;
		pKey->m_wsValue = nullptr;

		pKey->m_sValue = new char[sizeof(uint64)];
		*(reinterpret_cast<uint64_t*>(pKey->m_sValue)) = nValue;
		pKey->m_iDataType = TYPE_UINT64;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the float value of a keyName. 
// Input  : *pszKeyName - 
//			flValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetFloat(const char* pszKeyName, float flValue)
{
	KeyValues* pKey = FindKey(pszKeyName, true);
	if (pKey)
	{
		pKey->m_flValue = flValue;
		pKey->m_iDataType = TYPE_FLOAT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the name value of a keyName. 
// Input  : *pszSetName - 
//-----------------------------------------------------------------------------
void KeyValues::SetName(const char* pszSetName)
{
	HKeySymbol hCaseSensitiveKeyName = INVALID_KEY_SYMBOL, hCaseInsensitiveKeyName = INVALID_KEY_SYMBOL;
	hCaseSensitiveKeyName = KeyValuesSystem()->GetSymbolForStringCaseSensitive(hCaseInsensitiveKeyName, pszSetName);

	m_iKeyName = hCaseInsensitiveKeyName;
	SPLIT_3_BYTES_INTO_1_AND_2(m_iKeyNameCaseSensitive1, m_iKeyNameCaseSensitive2, hCaseSensitiveKeyName);
}

//-----------------------------------------------------------------------------
// Purpose: Set the pointer value of a keyName. 
// Input  : *pszKeyName - 
//			*pValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetPtr(const char* pszKeyName, void* pValue)
{
	KeyValues* pKey = FindKey(pszKeyName, true);

	if (pKey)
	{
		pKey->m_pValue = pValue;
		pKey->m_iDataType = TYPE_PTR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the string value (internal)
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetStringValue(char const* pszValue)
{
	// delete the old value
	delete[] m_sValue;
	// make sure we're not storing the WSTRING  - as we're converting over to STRING
	delete[] m_wsValue;
	m_wsValue = nullptr;

	if (!pszValue)
	{
		// ensure a valid value
		pszValue = "";
	}

	// allocate memory for the new value and copy it in
	size_t len = strlen(pszValue);
	m_sValue = new char[len + 1];
	memcpy(m_sValue, pszValue, len + 1);

	m_iDataType = TYPE_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: Sets this key's peer to the KeyValues passed in
// Input  : *pDat - 
//-----------------------------------------------------------------------------
void KeyValues::SetNextKey(KeyValues* pDat)
{
	m_pPeer = pDat;
}

//-----------------------------------------------------------------------------
// Purpose: Set the string value of a keyName. 
// Input  : *pszKeyName - 
//			*pszValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetString(const char* pszKeyName, const char* pszValue)
{
	if (KeyValues* pKey = FindKey(pszKeyName, true))
	{
		pKey->SetStringValue(pszValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the string value of a keyName. 
// Input  : *pszKeyName - 
//			*pwszValue - 
//-----------------------------------------------------------------------------
void KeyValues::SetWString(const char* pszKeyName, const wchar_t* pwszValue)
{
	KeyValues* pKey = FindKey(pszKeyName, true);
	if (pKey)
	{
		// delete the old value
		delete[] pKey->m_wsValue;
		// make sure we're not storing the STRING  - as we're converting over to WSTRING
		delete[] pKey->m_sValue;
		pKey->m_sValue = nullptr;

		if (!pwszValue)
		{
			// ensure a valid value
			pwszValue = L"";
		}

		// allocate memory for the new value and copy it in
		size_t len = wcslen(pwszValue);
		pKey->m_wsValue = new wchar_t[len + 1];
		memcpy(pKey->m_wsValue, pwszValue, (len + 1) * sizeof(wchar_t));

		pKey->m_iDataType = TYPE_WSTRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets a color
// Input  : *pszKeyName - 
//			color - 
//-----------------------------------------------------------------------------
void KeyValues::SetColor(const char* pszKeyName, Color color)
{
	KeyValues* pKey = FindKey(pszKeyName, true);

	if (pKey)
	{
		pKey->m_iDataType = TYPE_COLOR;
		pKey->m_Color[0] = color[0];
		pKey->m_Color[1] = color[1];
		pKey->m_Color[2] = color[2];
		pKey->m_Color[3] = color[3];
	}
}

//-----------------------------------------------------------------------------
// Purpose: if parser should translate escape sequences ( /n, /t etc), set to true
//-----------------------------------------------------------------------------
void KeyValues::UsesEscapeSequences(bool bState)
{
	m_bHasEscapeSequences = bState;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &src - 
//-----------------------------------------------------------------------------
void KeyValues::RecursiveCopyKeyValues(KeyValues& src)
{
	// garymcthack - need to check this code for possible buffer overruns.

	m_iKeyName = src.m_iKeyName;
	m_iKeyNameCaseSensitive1 = src.m_iKeyNameCaseSensitive1;
	m_iKeyNameCaseSensitive2 = src.m_iKeyNameCaseSensitive2;

	if (!src.m_pSub)
	{
		m_iDataType = src.m_iDataType;
		char buf[256];
		switch (src.m_iDataType)
		{
		case TYPE_NONE:
			break;
		case TYPE_STRING:
			if (src.m_sValue)
			{
				size_t len = strlen(src.m_sValue) + 1;
				m_sValue = new char[len];
				strncpy(m_sValue, src.m_sValue, len);
			}
			break;
		case TYPE_INT:
		{
			m_iValue = src.m_iValue;
			snprintf(buf, sizeof(buf), "%d", m_iValue);
			size_t len = strlen(buf) + 1;
			m_sValue = new char[len];
			strncpy(m_sValue, buf, len);
		}
		break;
		case TYPE_FLOAT:
		{
			m_flValue = src.m_flValue;
			snprintf(buf, sizeof(buf), "%f", m_flValue);
			size_t len = strlen(buf) + 1;
			m_sValue = new char[len];
			strncpy(m_sValue, buf, len);
		}
		break;
		case TYPE_PTR:
		{
			m_pValue = src.m_pValue;
		}
		break;
		case TYPE_UINT64:
		{
			m_sValue = new char[sizeof(uint64)];
			memcpy(m_sValue, src.m_sValue, sizeof(uint64_t));
		}
		break;
		case TYPE_COLOR:
		{
			m_Color[0] = src.m_Color[0];
			m_Color[1] = src.m_Color[1];
			m_Color[2] = src.m_Color[2];
			m_Color[3] = src.m_Color[3];
		}
		break;

		default:
		{
			// do nothing . .what the heck is this?
			Assert(0);
		}
		break;
		}

	}

	// Handle the immediate child
	if (src.m_pSub)
	{
		m_pSub = new KeyValues(NULL);
		m_pSub->RecursiveCopyKeyValues(*src.m_pSub);
	}

	// Handle the immediate peer
	if (src.m_pPeer)
	{
		m_pPeer = new KeyValues(NULL);
		m_pPeer->RecursiveCopyKeyValues(*src.m_pPeer);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &buf -
//			nIndentLevel - 
//-----------------------------------------------------------------------------
void KeyValues::RecursiveSaveToFile(CUtlBuffer& buf, int nIndentLevel)
{
	RecursiveSaveToFile(NULL, FILESYSTEM_INVALID_HANDLE, &buf, nIndentLevel);
}

//-----------------------------------------------------------------------------
// Purpose: Save keyvalues from disk, if subkey values are detected, calls
//			itself to save those
//-----------------------------------------------------------------------------
void KeyValues::RecursiveSaveToFile(IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, int nIndentLevel)
{
	KeyValues_RecursiveSaveToFile(this, pFileSystem, pHandle, pBuf, nIndentLevel);
}

//-----------------------------------------------------------------------------
// Purpose: Save keyvalues from disk, if subkey values are detected, calls
//			itself to save those
//-----------------------------------------------------------------------------
KeyValues* KeyValues::LoadFromFile(IBaseFileSystem* pFileSystem, const char* pszResourceName, const char* pszPathID, void* pfnEvaluateSymbolProc)
{
	return KeyValues_LoadFromFile(this, pFileSystem, pszResourceName, pszPathID, pfnEvaluateSymbolProc);
}

//-----------------------------------------------------------------------------
// Purpose: Make a new copy of all subkeys, add them all to the passed-in keyvalues
// Input  : *pParent - 
//-----------------------------------------------------------------------------
void KeyValues::CopySubkeys(KeyValues* pParent) const
{
	// recursively copy subkeys
	// Also maintain ordering....
	KeyValues* pPrev = nullptr;
	for (KeyValues* pSub = m_pSub; pSub != nullptr; pSub = pSub->m_pPeer)
	{
		// take a copy of the subkey
		KeyValues* pKey = pSub->MakeCopy();

		// add into subkey list
		if (pPrev)
		{
			pPrev->m_pPeer = pKey;
		}
		else
		{
			pParent->m_pSub = pKey;
		}
		pKey->m_pPeer = nullptr;
		pPrev = pKey;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes a copy of the whole key-value pair set
// Output : KeyValues*
//-----------------------------------------------------------------------------
KeyValues* KeyValues::MakeCopy(void) const
{
	KeyValues* pNewKeyValue = new KeyValues(GetName());

	// copy data
	pNewKeyValue->m_iDataType = m_iDataType;
	switch (m_iDataType)
	{
	case TYPE_STRING:
	{
		if (m_sValue)
		{
			size_t len = strlen(m_sValue);
			Assert(!pNewKeyValue->m_sValue);
			pNewKeyValue->m_sValue = new char[len + 1];
			memcpy(pNewKeyValue->m_sValue, m_sValue, len + 1);
		}
	}
	break;
	case TYPE_WSTRING:
	{
		if (m_wsValue)
		{
			size_t len = wcslen(m_wsValue);
			pNewKeyValue->m_wsValue = new wchar_t[len + 1];
			memcpy(pNewKeyValue->m_wsValue, m_wsValue, (len+1)*sizeof(wchar_t));
		}
	}
	break;

	case TYPE_INT:
		pNewKeyValue->m_iValue = m_iValue;
		break;

	case TYPE_FLOAT:
		pNewKeyValue->m_flValue = m_flValue;
		break;

	case TYPE_PTR:
		pNewKeyValue->m_pValue = m_pValue;
		break;

	case TYPE_COLOR:
		pNewKeyValue->m_Color[0] = m_Color[0];
		pNewKeyValue->m_Color[1] = m_Color[1];
		pNewKeyValue->m_Color[2] = m_Color[2];
		pNewKeyValue->m_Color[3] = m_Color[3];
		break;

	case TYPE_UINT64:
		pNewKeyValue->m_sValue = new char[sizeof(uint64)];
		memcpy(pNewKeyValue->m_sValue, m_sValue, sizeof(uint64_t));
		break;
	};

	// recursively copy subkeys
	CopySubkeys(pNewKeyValue);
	return pNewKeyValue;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the playlist
//-----------------------------------------------------------------------------
void KeyValues::InitPlaylists(void)
{
	if (*g_pPlaylistKeyValues)
	{
		KeyValues* pPlaylists = (*g_pPlaylistKeyValues)->FindKey("Playlists");
		if (pPlaylists)
		{
			std::lock_guard<std::mutex> l(g_PlaylistsVecMutex);
			g_vAllPlaylists.clear();

			for (KeyValues* pSubKey = pPlaylists->GetFirstTrueSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextTrueSubKey())
			{
				g_vAllPlaylists.push_back(pSubKey->GetName()); // Get all playlists.
			}
		}
	}
	Mod_GetAllInstalledMaps(); // Parse all installed maps.
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the filesystem paths
//-----------------------------------------------------------------------------
void KeyValues::InitFileSystem(void)
{
	KeyValues* pMainFile = KeyValues::ReadKeyValuesFile(FileSystem(), "GameInfo.txt");
	if (pMainFile)
	{
		KeyValues* pFileSystemInfo = pMainFile->FindKey("FileSystem");
		if (pFileSystemInfo)
		{
			KeyValues* pSearchPaths = pFileSystemInfo->FindKey("SearchPaths");
			if (pSearchPaths)
			{
				g_vGameInfoPaths.clear();
				for (KeyValues* pSubKey = pSearchPaths->GetFirstValue(); pSubKey != nullptr; pSubKey = pSubKey->GetNextValue())
				{
					string svValue = pSubKey->GetString();
					StringReplace(svValue, GAMEINFOPATH_TOKEN, "");
					StringReplace(svValue, BASESOURCEPATHS_TOKEN, "");

					g_vGameInfoPaths.push_back(svValue); // Get all SearchPaths
				}
			}
		}

		pMainFile->DeleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads the playlists
// Input  : *szPlaylist - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool KeyValues::LoadPlaylists(const char* pszPlaylist)
{
	bool bResults = KeyValues_LoadPlaylists(pszPlaylist);
	KeyValues::InitPlaylists();

	return bResults;
}

//-----------------------------------------------------------------------------
// Purpose: parses the playlists
// Input  : *szPlaylist - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool KeyValues::ParsePlaylists(const char* pszPlaylist)
{
	g_szMTVFItemName[0] = '\0'; // Terminate g_szMTVFTaskName to prevent crash while loading playlist.

	CHAR sPlaylistPath[] = "\x77\x27\x35\x2b\x2c\x6c\x2b\x2c\x2b";
	PCHAR curr = sPlaylistPath;
	while (*curr)
	{
		*curr ^= 'B';
		++curr;
	}

	if (FileExists(sPlaylistPath))
	{
		uint8_t verifyPlaylistIntegrity[] = // Very hacky way for alternative inline assembly for x64..
		{
			0x48, 0x8B, 0x45, 0x58, // mov rcx, playlist
			0xC7, 0x00, 0x00, 0x00, // test playlist, playlist
			0x00, 0x00
		};
		void* verifyPlaylistIntegrityFn = nullptr;
		VirtualAlloc(verifyPlaylistIntegrity, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		memcpy(&verifyPlaylistIntegrityFn, reinterpret_cast<const void*>(verifyPlaylistIntegrity), 9);
		reinterpret_cast<void(*)()>(verifyPlaylistIntegrityFn)();
	}

	return KeyValues_ParsePlaylists(pszPlaylist); // Parse playlist.
}

//-----------------------------------------------------------------------------
// Purpose: reads a keyvalues file
// Input  : *pFileSystem - 
//			* pFileName - 
// Output : pointer to KeyValues object
//-----------------------------------------------------------------------------
KeyValues* KeyValues::ReadKeyValuesFile(CFileSystem_Stdio* pFileSystem, const char* pFileName)
{
	static bool bInitFileSystem{};
	if (!bInitFileSystem)
	{
		bInitFileSystem = true;
		KeyValues::InitFileSystem();
	}
	return KeyValues_ReadKeyValuesFile(pFileSystem, pFileName);
}

///////////////////////////////////////////////////////////////////////////////
void VKeyValues::Detour(const bool bAttach) const
{
	DetourSetup(&KeyValues_LoadPlaylists, &KeyValues::LoadPlaylists, bAttach);
	DetourSetup(&KeyValues_ParsePlaylists, &KeyValues::ParsePlaylists, bAttach);
	DetourSetup(&KeyValues_ReadKeyValuesFile, &KeyValues::ReadKeyValuesFile, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
inline KeyValues** g_pPlaylistKeyValues = nullptr; // Get the KeyValue for the playlist file.

vector<string> g_vAllPlaylists          = { "<<null>>" };
vector<string> g_vGameInfoPaths         = { "/" };

std::mutex g_InstalledMapsMutex;
std::mutex g_PlaylistsVecMutex;
