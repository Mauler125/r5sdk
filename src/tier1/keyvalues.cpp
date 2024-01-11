//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/memstd.h"
#include "tier1/strtools.h"
#include "tier1/keyvalues.h"
#include "tier1/kvleaktrace.h"
#include "tier1/kverrorstack.h"
#include "tier1/kverrorcontext.h"
#include "tier1/kvtokenreader.h"
#include "vstdlib/keyvaluessystem.h"
#include "public/ifilesystem.h"
#include "mathlib/color.h"
#include "rtech/stryder/stryder.h"
#include "engine/sys_dll2.h"
#include "engine/cmodel_bsp.h"

static const char* s_LastFileLoadingFrom = "unknown"; // just needed for error messages
CExpressionEvaluator g_ExpressionEvaluator;

#define INTERNALWRITE( pData, nLen ) InternalWrite( pFileSystem, pHandle, pBuf, pData, nLen )

#define MAKE_3_BYTES_FROM_1_AND_2( x1, x2 ) (( (( uint16_t )x2) << 8 ) | (uint8_t)(x1))
#define SPLIT_3_BYTES_INTO_1_AND_2( x1, x2, x3 ) do { x1 = (uint8)(x3); x2 = (uint16)( (x3) >> 8 ); } while( 0 )

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
// Purpose: looks up a key by symbol name
//-----------------------------------------------------------------------------
KeyValues* KeyValues::FindKey(int keySymbol) const
{
	AssertMsg(this, "Member function called on NULL KeyValues");
	for (KeyValues* dat = this ? m_pSub : NULL; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_iKeyName == (uint32)keySymbol)
			return dat;
	}

	return NULL;
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
// Purpose: Get the symbol name of the current key section
//-----------------------------------------------------------------------------
int KeyValues::GetNameSymbol() const
{
	AssertMsg(this, "Member function called on NULL KeyValues");
	return this ? m_iKeyName : INVALID_KEY_SYMBOL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the symbol name of the current key section case sensitive
//-----------------------------------------------------------------------------
int KeyValues::GetNameSymbolCaseSensitive() const
{
	AssertMsg(this, "Member function called on NULL KeyValues");
	return this ? MAKE_3_BYTES_FROM_1_AND_2(m_iKeyNameCaseSensitive1, m_iKeyNameCaseSensitive2) : INVALID_KEY_SYMBOL;
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
// Purpose: Write out keyvalue data
//-----------------------------------------------------------------------------
void KeyValues::InternalWrite(IBaseFileSystem* filesystem, FileHandle_t f, CUtlBuffer* pBuf, const void* pData, ssize_t len)
{
	if (filesystem)
	{
		filesystem->Write(pData, len, f);
	}

	if (pBuf)
	{
		pBuf->Put(pData, len);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Write out a set of indenting
//-----------------------------------------------------------------------------
void KeyValues::WriteIndents(IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, int nIndentLevel)
{
	for (int i = 0; i < nIndentLevel; i++)
	{
		INTERNALWRITE("\t", 1);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Write out a string where we convert the double quotes to backslash double quote
//-----------------------------------------------------------------------------
void KeyValues::WriteConvertedString(IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, const char* pszString)
{
	// handle double quote chars within the string
	// the worst possible case is that the whole string is quotes
	size_t len = V_strlen(pszString);
	char* convertedString = (char*)alloca((len + 1) * sizeof(char) * 2);
	size_t j = 0;
	for (size_t i = 0; i <= len; i++)
	{
		if (pszString[i] == '\"')
		{
			convertedString[j] = '\\';
			j++;
		}
		else if (m_bHasEscapeSequences && pszString[i] == '\\')
		{
			convertedString[j] = '\\';
			j++;
		}
		convertedString[j] = pszString[i];
		j++;
	}

	INTERNALWRITE(convertedString, V_strlen(convertedString));
}

//-----------------------------------------------------------------------------
// Purpose: Save keyvalues to disk, if subkey values are detected, calls
//			itself to save those
//-----------------------------------------------------------------------------
void KeyValues::RecursiveSaveToFile(IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, int nIndentLevel)
{
	// write header
	WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel);
	INTERNALWRITE("\"", 1);
	WriteConvertedString(pFileSystem, pHandle, pBuf, GetName());
	INTERNALWRITE("\"\n", 2);
	WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel);
	INTERNALWRITE("{\n", 2);

	// loop through all our keys writing them to disk
	for (KeyValues* dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_pSub)
		{
			dat->RecursiveSaveToFile(pFileSystem, pHandle, pBuf, nIndentLevel + 1);
		}
		else
		{
			// only write non-empty keys
			switch (dat->m_iDataType)
			{
			case TYPE_STRING:
			{
				if (dat->m_sValue && *(dat->m_sValue))
				{
					WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel + 1);
					INTERNALWRITE("\"", 1);
					WriteConvertedString(pFileSystem, pHandle, pBuf, dat->GetName());
					INTERNALWRITE("\"\t\t\"", 4);

					WriteConvertedString(pFileSystem, pHandle, pBuf, dat->m_sValue);

					INTERNALWRITE("\"\n", 2);
				}
				break;
			}
			case TYPE_WSTRING:
			{
				if (dat->m_wsValue)
				{
					static char buf[KEYVALUES_TOKEN_SIZE];
					// make sure we have enough space
					int result = V_UnicodeToUTF8(dat->m_wsValue, buf, KEYVALUES_TOKEN_SIZE);
					if (result)
					{
						WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel + 1);
						INTERNALWRITE("\"", 1);
						INTERNALWRITE(dat->GetName(), V_strlen(dat->GetName()));
						INTERNALWRITE("\"\t\t\"", 4);

						WriteConvertedString(pFileSystem, pHandle, pBuf, buf);

						INTERNALWRITE("\"\n", 2);
					}
				}
				break;
			}

			case TYPE_INT:
			{
				WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel + 1);
				INTERNALWRITE("\"", 1);
				INTERNALWRITE(dat->GetName(), V_strlen(dat->GetName()));
				INTERNALWRITE("\"\t\t\"", 4);

				char buf[32];
				V_snprintf(buf, sizeof(buf), "%d", dat->m_iValue);

				INTERNALWRITE(buf, V_strlen(buf));
				INTERNALWRITE("\"\n", 2);
				break;
			}

			case TYPE_UINT64:
			{
				WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel + 1);
				INTERNALWRITE("\"", 1);
				INTERNALWRITE(dat->GetName(), V_strlen(dat->GetName()));
				INTERNALWRITE("\"\t\t\"", 4);

				char buf[32];
				// write "0x" + 16 char 0-padded hex encoded 64 bit value
				V_snprintf(buf, sizeof(buf), "0x%016llX", *((uint64*)dat->m_sValue));

				INTERNALWRITE(buf, V_strlen(buf));
				INTERNALWRITE("\"\n", 2);
				break;
			}

			case TYPE_FLOAT:
			{
				WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel + 1);
				INTERNALWRITE("\"", 1);
				INTERNALWRITE(dat->GetName(), V_strlen(dat->GetName()));
				INTERNALWRITE("\"\t\t\"", 4);

				char buf[48];
				V_snprintf(buf, sizeof(buf), "%f", dat->m_flValue);

				INTERNALWRITE(buf, V_strlen(buf));
				INTERNALWRITE("\"\n", 2);
				break;
			}
			case TYPE_COLOR:
				DevMsg(eDLL_T::COMMON, "%s: TODO, missing code for TYPE_COLOR.\n", __FUNCTION__);
				break;

			default:
				break;
			}
		}
	}

	// write tail
	WriteIndents(pFileSystem, pHandle, pBuf, nIndentLevel);
	INTERNALWRITE("}\n", 2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void KeyValues::RecursiveLoadFromBuffer(char const* resourceName, CKeyValuesTokenReader& tokenReader, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	CKeyErrorContext errorReport(GetNameSymbolCaseSensitive());
	bool wasQuoted;
	bool wasConditional;
	if (errorReport.GetStackLevel() > 100)
	{
		g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  recursion overflow");
		return;
	}

	// keep this out of the stack until a key is parsed
	CKeyErrorContext errorKey(INVALID_KEY_SYMBOL);

	// Locate the last child.  (Almost always, we will not have any children.)
	// We maintain the pointer to the last child here, so we don't have to re-locate
	// it each time we append the next subkey, which causes O(N^2) time
	KeyValues* pLastChild = FindLastSubKey();

	// Keep parsing until we hit the closing brace which terminates this block, or a parse error
	while (1)
	{
		bool bAccepted = true;

		// get the key name
		const char* name = tokenReader.ReadToken(wasQuoted, wasConditional);

		if (!name)	// EOF stop reading
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got EOF instead of keyname");
			break;
		}

		if (!*name) // empty token, maybe "" or EOF
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got empty keyname");
			break;
		}

		if (*name == '}' && !wasQuoted)	// top level closed, stop reading
			break;

		// Always create the key; note that this could potentially
		// cause some duplication, but that's what we want sometimes
		KeyValues* dat = CreateKeyUsingKnownLastChild(name, pLastChild);

		errorKey.Reset(dat->GetNameSymbolCaseSensitive());

		// get the value
		const char* value = tokenReader.ReadToken(wasQuoted, wasConditional);

		bool bFoundConditional = wasConditional;
		if (wasConditional && value)
		{
			bAccepted = EvaluateConditional(value, pfnEvaluateSymbolProc);

			// get the real value
			value = tokenReader.ReadToken(wasQuoted, wasConditional);
		}

		if (!value)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got NULL key");
			break;
		}

		// support the '=' as an assignment, makes multiple-keys-on-one-line easier to read in a keyvalues file
		if (*value == '=' && !wasQuoted)
		{
			// just skip over it
			value = tokenReader.ReadToken(wasQuoted, wasConditional);
			bFoundConditional = wasConditional;
			if (wasConditional && value)
			{
				bAccepted = EvaluateConditional(value, pfnEvaluateSymbolProc);

				// get the real value
				value = tokenReader.ReadToken(wasQuoted, wasConditional);
			}

			if (bFoundConditional && bAccepted)
			{
				// if there is a conditional key see if we already have the key defined and blow it away, last one in the list wins
				KeyValues* pExistingKey = this->FindKey(dat->GetNameSymbol());
				if (pExistingKey && pExistingKey != dat)
				{
					this->RemoveSubKey(pExistingKey);
					pExistingKey->DeleteThis();
				}
			}
		}

		if (!value)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got NULL key");
			break;
		}

		if (*value == '}' && !wasQuoted)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got } in key");
			break;
		}

		if (*value == '{' && !wasQuoted)
		{
			// this isn't a key, it's a section
			errorKey.Reset(INVALID_KEY_SYMBOL);
			// sub value list
			dat->RecursiveLoadFromBuffer(resourceName, tokenReader, pfnEvaluateSymbolProc);
		}
		else
		{
			if (wasConditional)
			{
				g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got conditional between key and value");
				break;
			}

			if (dat->m_sValue)
			{
				delete[] dat->m_sValue;
				dat->m_sValue = NULL;
			}

			size_t len = V_strlen(value);

			// Here, let's determine if we got a float or an int....
			char* pIEnd;	// pos where int scan ended
			char* pFEnd;	// pos where float scan ended
			const char* pSEnd = value + len; // pos where token ends

			const long lval = strtol(value, &pIEnd, 10);
			const float fval = (float)strtod(value, &pFEnd);
			const bool bOverflow = (lval == LONG_MAX || lval == LONG_MIN) && errno == ERANGE;
#ifdef POSIX
			// strtod supports hex representation in strings under posix but we DON'T
			// want that support in keyvalues, so undo it here if needed
			if (len > 1 && tolower(value[1]) == 'x')
			{
				fval = 0.0f;
				pFEnd = (char*)value;
			}
#endif

			if (*value == 0)
			{
				dat->m_iDataType = TYPE_STRING;
			}
			else if ((18 == len) && (value[0] == '0') && (value[1] == 'x'))
			{
				// an 18-byte value prefixed with "0x" (followed by 16 hex digits) is an int64 value
				int64 retVal = 0;
				for (int i = 2; i < 2 + 16; i++)
				{
					char digit = value[i];
					if (digit >= 'a')
						digit -= 'a' - ('9' + 1);
					else
						if (digit >= 'A')
							digit -= 'A' - ('9' + 1);
					retVal = (retVal * 16) + (digit - '0');
				}
				dat->m_sValue = new char[sizeof(uint64)];
				*((uint64*)dat->m_sValue) = retVal;
				dat->m_iDataType = TYPE_UINT64;
			}
			else if ((pFEnd > pIEnd) && (pFEnd == pSEnd))
			{
				dat->m_flValue = fval;
				dat->m_iDataType = TYPE_FLOAT;
			}
			else if (pIEnd == pSEnd && !bOverflow)
			{
				dat->m_iValue = static_cast<int>(lval);
				dat->m_iDataType = TYPE_INT;
			}
			else
			{
				dat->m_iDataType = TYPE_STRING;
			}

			if (dat->m_iDataType == TYPE_STRING)
			{
				// copy in the string information
				dat->m_sValue = new char[len + 1];
				memcpy(dat->m_sValue, value, len + 1);
			}

			// Look ahead one token for a conditional tag
			const char* peek = tokenReader.ReadToken(wasQuoted, wasConditional);
			if (wasConditional)
			{
				bAccepted = EvaluateConditional(peek, pfnEvaluateSymbolProc);
			}
			else
			{
				tokenReader.SeekBackOneToken();
			}
		}

		Assert(dat->m_pPeer == NULL);
		if (bAccepted)
		{
			Assert(pLastChild == NULL || pLastChild->m_pPeer == dat);
			pLastChild = dat;
		}
		else
		{
			//this->RemoveSubKey( dat );
			if (pLastChild == NULL)
			{
				Assert(this->m_pSub == dat);
				this->m_pSub = NULL;
			}
			else
			{
				Assert(pLastChild->m_pPeer == dat);
				pLastChild->m_pPeer = NULL;
			}

			delete dat;
			dat = NULL;
		}
	}
}

// prevent two threads from entering this at the same time and trying to share the global error reporting and parse buffers
static CThreadFastMutex g_KVMutex;
//-----------------------------------------------------------------------------
// Read from a buffer...
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromBuffer(char const* resourceName, CUtlBuffer& buf, IBaseFileSystem* pFileSystem, const char* pPathID, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	AUTO_LOCK(g_KVMutex);

	//if (IsGameConsole())
	//{
	//	// Let's not crash if the buffer is empty
	//	unsigned char* pData = buf.Size() > 0 ? (unsigned char*)buf.PeekGet() : NULL;
	//	if (pData && (unsigned int)pData[0] == KV_BINARY_POOLED_FORMAT)
	//	{
	//		// skip past binary marker
	//		buf.GetUnsignedChar();
	//		// get the pool identifier, allows the fs to bind the expected string pool
	//		unsigned int poolKey = buf.GetUnsignedInt();

	//		RemoveEverything();
	//		Init();

	//		return ReadAsBinaryPooledFormat(buf, pFileSystem, poolKey, pfnEvaluateSymbolProc);
	//	}
	//}

	KeyValues* pPreviousKey = NULL;
	KeyValues* pCurrentKey = this;
	CUtlVector< KeyValues* > includedKeys;
	CUtlVector< KeyValues* > baseKeys;
	bool wasQuoted;
	bool wasConditional;
	CKeyValuesTokenReader tokenReader(this, buf);

	g_KeyValuesErrorStack.SetFilename(resourceName);
	do
	{
		bool bAccepted = true;

		// the first thing must be a key
		const char* s = tokenReader.ReadToken(wasQuoted, wasConditional);
		if (!buf.IsValid() || !s)
			break;

		if (!wasQuoted && *s == '\0')
		{
			// non quoted empty strings stop parsing
			// quoted empty strings are allowed to support unnnamed KV sections
			break;
		}

		if (!V_stricmp(s, "#include"))	// special include macro (not a key name)
		{
			s = tokenReader.ReadToken(wasQuoted, wasConditional);
			// Name of subfile to load is now in s

			if (!s || *s == 0)
			{
				g_KeyValuesErrorStack.ReportError("#include is NULL ");
			}
			else
			{
				ParseIncludedKeys(resourceName, s, pFileSystem, pPathID, includedKeys, pfnEvaluateSymbolProc);
			}

			continue;
		}
		else if (!V_stricmp(s, "#base"))
		{
			s = tokenReader.ReadToken(wasQuoted, wasConditional);
			// Name of subfile to load is now in s

			if (!s || *s == 0)
			{
				g_KeyValuesErrorStack.ReportError("#base is NULL ");
			}
			else
			{
				ParseIncludedKeys(resourceName, s, pFileSystem, pPathID, baseKeys, pfnEvaluateSymbolProc);
			}

			continue;
		}

		if (!pCurrentKey)
		{
			pCurrentKey = new KeyValues(s);
			Assert(pCurrentKey);

			pCurrentKey->UsesEscapeSequences(m_bHasEscapeSequences != 0); // same format has parent use

			if (pPreviousKey)
			{
				pPreviousKey->SetNextKey(pCurrentKey);
			}
		}
		else
		{
			pCurrentKey->SetName(s);
		}

		// get the '{'
		s = tokenReader.ReadToken(wasQuoted, wasConditional);

		if (wasConditional)
		{
			bAccepted = EvaluateConditional(s, pfnEvaluateSymbolProc);

			// Now get the '{'
			s = tokenReader.ReadToken(wasQuoted, wasConditional);
		}

		if (s && *s == '{' && !wasQuoted)
		{
			// header is valid so load the file
			pCurrentKey->RecursiveLoadFromBuffer(resourceName, tokenReader, pfnEvaluateSymbolProc);
		}
		else
		{
			g_KeyValuesErrorStack.ReportError("LoadFromBuffer: missing {");
		}

		if (!bAccepted)
		{
			if (pPreviousKey)
			{
				pPreviousKey->SetNextKey(NULL);
			}
			pCurrentKey->Clear();
		}
		else
		{
			pPreviousKey = pCurrentKey;
			pCurrentKey = NULL;
		}
	} while (buf.IsValid());

	AppendIncludedKeys(includedKeys);
	{
		// delete included keys!
		int i;
		for (i = includedKeys.Count() - 1; i > 0; i--)
		{
			KeyValues* kv = includedKeys[i];
			delete kv;
		}
	}

	MergeBaseKeys(baseKeys);
	{
		// delete base keys!
		int i;
		for (i = baseKeys.Count() - 1; i >= 0; i--)
		{
			KeyValues* kv = baseKeys[i];
			delete kv;
		}
	}

	bool bErrors = g_KeyValuesErrorStack.EncounteredAnyErrors();
	g_KeyValuesErrorStack.SetFilename("");
	g_KeyValuesErrorStack.ClearErrorFlag();
	return !bErrors;
}

//-----------------------------------------------------------------------------
// Read from a buffer...
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromBuffer(char const* resourceName, const char* pBuffer, IBaseFileSystem* pFileSystem, const char* pPathID, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	if (!pBuffer)
		return true;

	if (IsGameConsole() && (unsigned int)((unsigned char*)pBuffer)[0] == KV_BINARY_POOLED_FORMAT)
	{
		// bad, got a binary compiled KV file through an unexpected text path
		// not all paths support binary compiled kv, needs to get fixed
		// need to have caller supply buffer length (strlen not valid), this interface change was never plumbed
		Warning(eDLL_T::COMMON, "ERROR! Binary compiled KV '%s' in an unexpected handler\n", resourceName);
		Assert(0);
		return false;
	}

	size_t nLen = V_strlen(pBuffer);
	CUtlBuffer buf(pBuffer, nLen, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);
	// Translate Unicode files into UTF-8 before proceeding
	if (nLen > 2 && (uint8)pBuffer[0] == 0xFF && (uint8)pBuffer[1] == 0xFE)
	{
		int nUTF8Len = V_UnicodeToUTF8((wchar_t*)(pBuffer + 2), NULL, 0);
		char* pUTF8Buf = new char[nUTF8Len];
		V_UnicodeToUTF8((wchar_t*)(pBuffer + 2), pUTF8Buf, nUTF8Len);
		buf.AssumeMemory(pUTF8Buf, nUTF8Len, nUTF8Len, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);
	}
	return LoadFromBuffer(resourceName, buf, pFileSystem, pPathID, pfnEvaluateSymbolProc);
}

//-----------------------------------------------------------------------------
// Purpose: Load keyValues from disk
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromFile(IBaseFileSystem* filesystem, const char* resourceName, const char* pathID, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	//TM_ZONE_FILTERED( TELEMETRY_LEVEL0, 50, TMZF_NONE, "%s %s", __FUNCTION__, tmDynamicString( TELEMETRY_LEVEL0, resourceName ) );

	FileHandle_t f = filesystem->Open(resourceName, "rt", pathID);
	if (!f)
		return false;

	s_LastFileLoadingFrom = (char*)resourceName;

	// load file into a null-terminated buffer
	const ssize_t fileSize = filesystem->Size(f);
	std::unique_ptr<char[]> pBuf(new char[fileSize + 1]);

	const ssize_t nRead = filesystem->Read(pBuf.get(), fileSize, f);
	filesystem->Close(f);

	// TODO[ AMOS ]: unicode null terminate?
	pBuf[nRead] = '\0';

	return LoadFromBuffer(resourceName, pBuf.get(), filesystem, pathID, pfnEvaluateSymbolProc);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : includedKeys - 
//-----------------------------------------------------------------------------
void KeyValues::AppendIncludedKeys(CUtlVector< KeyValues* >& includedKeys)
{
	// Append any included keys, too...
	int includeCount = includedKeys.Count();
	int i;
	for (i = 0; i < includeCount; i++)
	{
		KeyValues* kv = includedKeys[i];
		Assert(kv);

		KeyValues* insertSpot = this;
		while (insertSpot->GetNextKey())
		{
			insertSpot = insertSpot->GetNextKey();
		}

		insertSpot->SetNextKey(kv);
	}
}

void KeyValues::ParseIncludedKeys(char const* resourceName, const char* filetoinclude,
	IBaseFileSystem* pFileSystem, const char* pPathID, CUtlVector< KeyValues* >& includedKeys, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	Assert(resourceName);
	Assert(filetoinclude);
	Assert(pFileSystem);

	// Load it...
	if (!pFileSystem)
	{
		return;
	}

	// Get relative subdirectory
	char fullpath[512];
	V_strncpy(fullpath, resourceName, sizeof(fullpath));

	// Strip off characters back to start or first /
	bool done = false;
	size_t len = V_strlen(fullpath);
	while (!done)
	{
		if (len == 0)
		{
			break;
		}

		if (fullpath[len - 1] == '\\' ||
			fullpath[len - 1] == '/')
		{
			break;
		}

		// zero it
		fullpath[len - 1] = 0;
		--len;
	}

	// Append included file
	V_strncat(fullpath, filetoinclude, sizeof(fullpath));

	KeyValues* newKV = new KeyValues(fullpath);

	// CUtlSymbol save = s_CurrentFileSymbol;	// did that had any use ???

	newKV->UsesEscapeSequences(m_bHasEscapeSequences != 0);	// use same format as parent

	if (newKV->LoadFromFile(pFileSystem, fullpath, pPathID, pfnEvaluateSymbolProc))
	{
		includedKeys.AddToTail(newKV);
	}
	else
	{
		DevMsg(eDLL_T::COMMON, "%s: Couldn't load included keyvalue file %s\n", __FUNCTION__, fullpath);
		delete newKV;
	}

	// s_CurrentFileSymbol = save;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseKeys - 
//-----------------------------------------------------------------------------
void KeyValues::MergeBaseKeys(CUtlVector< KeyValues* >& baseKeys)
{
	const int includeCount = baseKeys.Count();

	for (int i = 0; i < includeCount; i++)
	{
		KeyValues* kv = baseKeys[i];
		Assert(kv);

		RecursiveMergeKeyValues(kv);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseKV - keyvalues we're basing ourselves on
//-----------------------------------------------------------------------------
void KeyValues::RecursiveMergeKeyValues(KeyValues* baseKV)
{
	// Merge ourselves
	// we always want to keep our value, so nothing to do here

	// Now merge our children
	for (KeyValues* baseChild = baseKV->m_pSub; baseChild != NULL; baseChild = baseChild->m_pPeer)
	{
		// for each child in base, see if we have a matching kv

		bool bFoundMatch = false;

		// If we have a child by the same name, merge those keys
		for (KeyValues* newChild = m_pSub; newChild != NULL; newChild = newChild->m_pPeer)
		{
			if (!V_strcmp(baseChild->GetName(), newChild->GetName()))
			{
				newChild->RecursiveMergeKeyValues(baseChild);
				bFoundMatch = true;
				break;
			}
		}

		// If not merged, append this key
		if (!bFoundMatch)
		{
			KeyValues* dat = baseChild->MakeCopy();
			Assert(dat);
			AddSubKey(dat);
		}
	}
}

//-----------------------------------------------------------------------------
// Returns whether a keyvalues conditional expression string evaluates to true or false
//-----------------------------------------------------------------------------
bool KeyValues::EvaluateConditional(const char* pExpressionString, GetSymbolProc_t pfnEvaluateSymbolProc)
{
	// evaluate the infix expression, calling the symbol proc to resolve each symbol's value
	bool bResult = false;
	const bool bValid = g_ExpressionEvaluator.Evaluate(bResult, pExpressionString, pfnEvaluateSymbolProc);
	if (!bValid)
	{
		g_KeyValuesErrorStack.ReportError("KV Conditional Evaluation Error");
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
KeyValues* KeyValues::CreateKeyUsingKnownLastChild(const char* keyName, KeyValues* pLastChild)
{
	// Create a new key
	KeyValues* dat = new KeyValues(keyName);

	dat->UsesEscapeSequences(m_bHasEscapeSequences != 0); // use same format as parent does

	// add into subkey list
	AddSubkeyUsingKnownLastChild(dat, pLastChild);

	return dat;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void KeyValues::AddSubkeyUsingKnownLastChild(KeyValues* pSubkey, KeyValues* pLastChild)
{
	// Make sure the subkey isn't a child of some other keyvalues
	Assert(pSubkey != NULL);
	Assert(pSubkey->m_pPeer == NULL);

	// Empty child list?
	if (pLastChild == NULL)
	{
		Assert(m_pSub == NULL);
		m_pSub = pSubkey;
	}
	else
	{
		Assert(m_pSub != NULL);
		Assert(pLastChild->m_pPeer == NULL);

		pLastChild->SetNextKey(pSubkey);
	}
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
