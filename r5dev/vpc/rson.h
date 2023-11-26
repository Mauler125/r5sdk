#pragma once
#include "mathlib/color.h"
#include "tier0/tslist.h"
#include "tier1/utlbuffer.h"
#include "public/ifilesystem.h"
#include "filesystem/filesystem.h"

class RSON
{
public:
	enum eFieldType
	{
		RSON_NULL = 0x1,
		RSON_STRING = 0x2,
		RSON_VALUE = 0x4,
		RSON_OBJECT = 0x8,
		RSON_BOOLEAN = 0x10,
		RSON_INTEGER = 0x20,
		RSON_SIGNED_INTEGER = 0x40,
		RSON_UNSIGNED_INTEGER = 0x80,
		RSON_DOUBLE = 0x100,
		RSON_ARRAY = 0x1000,
	};

	struct Field_t;

	union Value_t
	{
		Field_t* pSubKey;
		char* pszString;
		__int64 integerValue;
	};

	// used for the root node of rson tree
	struct Node_t
	{
		eFieldType m_Type;
		int m_nValueCount;
		Value_t m_Value;

		Field_t* GetFirstSubKey()
		{
			if (m_Type & eFieldType::RSON_OBJECT)
				return m_Value.pSubKey;
			return NULL;
		};

		// does not support finding a key in a different level of the tree
		Field_t* FindKey(const char* pszKeyName)
		{
			if ((m_Type & eFieldType::RSON_OBJECT) == 0)
				return NULL;

			for (Field_t* pKey = GetFirstSubKey(); pKey != nullptr; pKey = pKey->GetNextKey())
			{
				if (!_stricmp(pKey->m_pszName, pszKeyName))
					return pKey;
			}

			return NULL;
		}
	};

	// used for every other field of the rson tree
	struct Field_t
	{
		char* m_pszName;
		Node_t m_Node;
		Field_t* m_pNext;
		Field_t* m_pPrev;

		Field_t* GetNextKey() { return m_pNext; };
		Field_t* GetLastKey() { return m_pPrev; };

		Field_t* GetFirstSubKey() { return m_Node.GetFirstSubKey(); };

		Field_t* FindKey(const char* pszKeyName) { return m_Node.FindKey(pszKeyName); };

		const char* GetString() { return (m_Node.m_Type == RSON_STRING) ? m_Node.m_Value.pszString : NULL; };
	};

public:
	static Node_t* LoadFromBuffer(const char* pszBufferName, char* pBuffer, eFieldType rootType);
	static Node_t* LoadFromFile(const char* pszFilePath, const char* pPathID = nullptr);
};
///////////////////////////////////////////////////////////////////////////////
inline CMemory p_RSON_LoadFromBuffer;
inline RSON::Node_t* (*RSON_LoadFromBuffer)(const char* bufName, char* buf, RSON::eFieldType rootType, __int64 a4, void* a5);

inline CMemory p_RSON_Free;
inline void (*RSON_Free)(RSON::Node_t* rson, CAlignedMemAlloc* allocator);

///////////////////////////////////////////////////////////////////////////////
class VRSON : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("RSON_LoadFromBuffer", p_RSON_LoadFromBuffer.GetPtr());
		LogFunAdr("RSON_Free", p_RSON_Free.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_RSON_LoadFromBuffer = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 89 45 60 48 8B D8").FollowNearCallSelf();
		RSON_LoadFromBuffer = p_RSON_LoadFromBuffer.RCast< RSON::Node_t* (*)(const char*, char*, RSON::eFieldType, __int64, void*)>();

		p_RSON_Free = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 83 EF 01 75 E7").FollowNearCallSelf();
		RSON_Free = p_RSON_Free.RCast<void(*)(RSON::Node_t*, CAlignedMemAlloc*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

