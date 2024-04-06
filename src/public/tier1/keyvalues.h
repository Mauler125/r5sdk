#pragma once
#include "mathlib/color.h"
#include "tier1/utlbuffer.h"
#include "tier1/exprevaluator.h"
#include "public/ifilesystem.h"

#define KEYVALUES_TOKEN_SIZE	(1024 * 32)

// single byte identifies a xbox kv file in binary format
// strings are pooled from a searchpath/zip mounted symbol table
#define KV_BINARY_POOLED_FORMAT 0xAA

//---------------------------------------------------------------------------------
// Purpose: Forward declarations
//---------------------------------------------------------------------------------
class KeyValues;
class CFileSystem_Stdio;
class IBaseFileSystem;
class CKeyValuesTokenReader;

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

#define FOR_EACH_SUBKEY( kvRoot, kvSubKey ) \
	for ( KeyValues * kvSubKey = kvRoot->GetFirstSubKey(); kvSubKey != NULL; kvSubKey = kvSubKey->GetNextKey() )

#define FOR_EACH_TRUE_SUBKEY( kvRoot, kvSubKey ) \
	for ( KeyValues * kvSubKey = kvRoot->GetFirstTrueSubKey(); kvSubKey != NULL; kvSubKey = kvSubKey->GetNextTrueSubKey() )

#define FOR_EACH_VALUE( kvRoot, kvValue ) \
	for ( KeyValues * kvValue = kvRoot->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue() )

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

	KeyValues* FindKey(int keySymbol) const;
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
	int GetNameSymbol() const;
	int GetNameSymbolCaseSensitive() const;
	int GetInt(const char* pszKeyName, int iDefaultValue);
	uint64_t GetUint64(const char* pszKeyName, uint64_t nDefaultValue);
	void* GetPtr(const char* pszKeyName, void* pDefaultValue);
	float GetFloat(const char* pszKeyName, float flDefaultValue);
	const char* GetString(const char* pszKeyName = nullptr, const char* pszDefaultValue = "");
	const wchar_t* GetWString(const char* pszKeyName = nullptr, const wchar_t* pwszDefaultValue = L"");
	Color GetColor(const char* pszKeyName, const Color& defaultColor);
	bool GetBool(const char* pszKeyName = nullptr, bool nDefaultValue = false) { return GetInt(pszKeyName, nDefaultValue ? 1 : 0) ? true : false; }
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
	void SetBool(const char* pszKeyName, bool bValue) { SetInt(pszKeyName, bValue ? 1 : 0); }
	void UsesEscapeSequences(bool bState);

	void RecursiveSaveToFile(CUtlBuffer& buf, int nIndentLevel);

	bool LoadFromFile(IBaseFileSystem* filesystem, const char* resourceName, const char* pathID, GetSymbolProc_t pfnEvaluateSymbolProc = nullptr);

	bool LoadFromBuffer(char const* resourceName, CUtlBuffer& buf, IBaseFileSystem* pFileSystem, const char* pPathID, GetSymbolProc_t pfnEvaluateSymbolProc = nullptr);
	bool LoadFromBuffer(char const* resourceName, const char* pBuffer, IBaseFileSystem* pFileSystem, const char* pPathID, GetSymbolProc_t pfnEvaluateSymbolProc = nullptr);

	// for handling #include "filename"
	void AppendIncludedKeys(CUtlVector< KeyValues* >& includedKeys);
	void ParseIncludedKeys(char const* resourceName, const char* filetoinclude,
		IBaseFileSystem* pFileSystem, const char* pPathID, CUtlVector< KeyValues* >& includedKeys, GetSymbolProc_t pfnEvaluateSymbolProc);

	// For handling #base "filename"
	void MergeBaseKeys(CUtlVector< KeyValues* >& baseKeys);
	void RecursiveMergeKeyValues(KeyValues* baseKV);

	void CopySubkeys(KeyValues* pParent) const;
	KeyValues* MakeCopy(void) const;

	KeyValues* CreateKeyUsingKnownLastChild(const char* keyName, KeyValues* pLastChild);
	void AddSubkeyUsingKnownLastChild(KeyValues* pSubKey, KeyValues* pLastChild);

private:
	void RecursiveSaveToFile(IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, int nIndentLevel);
	void RecursiveLoadFromBuffer(char const* resourceName, CKeyValuesTokenReader& tokenReader, GetSymbolProc_t pfnEvaluateSymbolProc);

	void RecursiveCopyKeyValues(KeyValues& src);

	// NOTE: If both filesystem and pBuf are non-null, it'll save to both of them.
	// If filesystem is null, it'll ignore f.
	void InternalWrite(IBaseFileSystem* filesystem, FileHandle_t f, CUtlBuffer* pBuf, const void* pData, ssize_t len);
	void WriteIndents(IBaseFileSystem* filesystem, FileHandle_t f, CUtlBuffer* pBuf, int indentLevel);
	void WriteConvertedString(IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, const char* pszString);

	bool EvaluateConditional(const char* pExpressionString, GetSymbolProc_t pfnEvaluateSymbolProc);

public:
	uint32_t         m_iKeyName               : 24;// 0x0000
	uint32_t         m_iKeyNameCaseSensitive1 : 8; // 0x0003
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
