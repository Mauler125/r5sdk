#pragma once
#include "patterns.h"

/////////////////////////////////////////////////////////////////////////////
// Classes and Structs

class CInputSystem
{
public:
	void EnableInput(bool bEnabled)// @0x14039F100 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void(__thiscall*)(CInputSystem*, bool);
		(*reinterpret_cast<OriginalFn**>(this))[10](this, bEnabled); 
	}

	void EnableMessagePump(bool bEnabled) // @0x14039F110 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void(__thiscall*)(CInputSystem*, bool);
		(*reinterpret_cast<OriginalFn**>(this))[11](this, bEnabled); 
	}

	bool IsButtonDown(ButtonCode_t Button) // @0x1403A0140 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = bool(__thiscall*)(CInputSystem*, ButtonCode_t);
		return (*reinterpret_cast<OriginalFn**>(this))[13](this, Button); 
	}

private:
	char pad_0000[16]; //0x0000
public:
	bool m_bEnabled; //0x0010 IsInputEnabled variable.
	bool m_bPumpEnabled; //0x0011 EnabledMessagePump variable.
};

typedef int HKeySymbol;

class CKeyValuesSystem // VTABLE @ 0x1413AA1E8 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:

	void RegisterSizeofKeyValues(__int64 size) //@0x1413AA1F0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void(__thiscall*)(CKeyValuesSystem*, __int64);
		(*reinterpret_cast<OriginalFn**>(this))[0](this, size);
	}

	void* AllocKeyValuesMemory(__int64 size) // @0x1413AA1F8 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void* (__thiscall*)(CKeyValuesSystem*, __int64);
		return (*reinterpret_cast<OriginalFn**>(this))[1](this, size);
	}

	void FreeKeyValuesMemory(void* pMem) // @0x1413AA200 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void(__thiscall*)(CKeyValuesSystem*, void*);
		(*reinterpret_cast<OriginalFn**>(this))[2](this, pMem);
	}

	HKeySymbol GetSymbolForString(const char* name, bool bCreate) // @0x1413AA208 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = HKeySymbol(__thiscall*)(CKeyValuesSystem*, const char*, bool);
		return (*reinterpret_cast<OriginalFn**>(this))[3](this, name, bCreate);
	}

	const char* GetStringForSymbol(HKeySymbol symbol) // @0x1413AA210 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = const char* (__thiscall*)(CKeyValuesSystem*, HKeySymbol);
		return (*reinterpret_cast<OriginalFn**>(this))[4](this, symbol);
	}

	//	void __fastcall CKeyValuesSystem::FreeKeyValuesMemory(CKeyValuesSystem* this_arg, void* ptr_mem_arg)
	//	{
	//		__int64* v2; // rax
	//		__int64 v4; // rax
	//		__int64* v5; // rax
	// 
	//		v2 = qword_14D40B538;
	//		if (!qword_14D40B538)
	//		{
	//			v2 = sub_140462930();
	//			qword_14D40B538 = v2;
	//		}
	//		v4 = (*(*v2 + 48))(v2, ptr_mem_arg);
	//		if (v4 > 0)
	//			CKeyValuesSystem::m_pMemPool -= v4;
	//		v5 = qword_14D40B538;
	//		if (!qword_14D40B538)
	//		{
	//			v5 = sub_140462930();
	//			qword_14D40B538 = v5;
	//		}
	//		(*(*v5 + 40))(v5, ptr_mem_arg);
	//	}

	// GetMemPool return a global variable called m_pMemPool it gets modified by AllocKeyValuesMemory and FreeKeyValuesMemory above you can see where the find it in FreeKeyValuesMemory.
	void* GetMemPool() // @0x1413AA228 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		return reinterpret_cast<void*>(0x14D412768); // May need to dereference is once more not sure right now.
	}

	void SetKeyValuesExpressionSymbol(const char* name, bool bValue) // @0x1413AA230 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void(__thiscall*)(CKeyValuesSystem*, const char*, bool);
		(*reinterpret_cast<OriginalFn**>(this))[8](this, name, bValue);
	}

	bool GetKeyValuesExpressionSymbol(const char* name) // @0x1413AA238 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = bool(__thiscall*)(CKeyValuesSystem*, const char*);
		return (*reinterpret_cast<OriginalFn**>(this))[9](this, name);
	}

	HKeySymbol GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char* name, bool bCreate) // @0x1413AA240 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = HKeySymbol(__thiscall*)(CKeyValuesSystem*, HKeySymbol&, const char*, bool);
		return (*reinterpret_cast<OriginalFn**>(this))[10](this, hCaseInsensitiveSymbol, name, bCreate);
	}

	// Datatypes aren't accurate. But full fill the actual byte distance.
public:
	void* vtable; // 0x0000
	__int64 m_iMaxKeyValuesSize; // 0x0008
private:
	char gap10[240]; // 0x0010
public:
	__int32 m_KvConditionalSymbolTable; // 0x0100
private:
	char gap104[4]; // 0x0104
public:
	__int64 field_108; // 0x0108
private:
	char gap110[32]; // 0x0110
public:
	int m_mutex; // 0x0130
};

class KeyValues
{
public:

	KeyValues* FindKey(const char* keyName, bool bCreate)
	{
		//	static auto func = reinterpret_cast<KeyValues*(__thiscall*)(KeyValues*, const char*, bool)>(p_KeyValues_FindKey);
		//	return func(this, keyName, bCreate);
		return nullptr;
	}

	int GetInt(const char* keyName, int defaultValue)
	{
		KeyValues* dat = FindKey(keyName, false);

		if (!dat)
			return defaultValue;

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

		return defaultValue;
	}

	void SetInt(const char* keyName, int iValue)
	{
		KeyValues* dat = FindKey(keyName, true);
		if (dat)
		{
			dat->m_iValue() = iValue;
			dat->m_iDataType = TYPE_INT;
		}
	}

	void SetFloat(const char* keyName, float flValue)
	{
		KeyValues* dat = FindKey(keyName, true);
		if (dat)
		{
			dat->m_flValue() = flValue;
			dat->m_iDataType = TYPE_FLOAT;
		}
	}

	// Compiler makes it so m_Value shares the offset spot with m_flValue that why we cast it like this.
	float& m_flValue()
	{
		static __int32 offset = 0x18;
		return *(float*)((std::uintptr_t)this + offset);
	}

	int& m_iValue()
	{
		static __int32 offset = 0x18;
		return *(int*)((std::uintptr_t)this + offset);
	}

public:
	unsigned __int32 m_iKeyName : 24; // 0x0000
	unsigned __int32 m_iKeyNameCaseSensitive : 8; // 0x0003
	char* m_sValue; // 0x0008
	wchar_t* m_wsValue; // 0x0010
	int m_Value; // 0x0018
private:
	char gap1C[12]; // 0x0020
public:
	char m_iDataType; // 0x0028
	unsigned __int16 m_iKeyNameCaseSensitive2; // 0x002A
	KeyValues* m_pPeer; // 0x0030
	KeyValues* m_pSub; // 0x0038
	KeyValues* m_pChain; // 0x0040
};

struct Vector3 // Implement the proper class of this at some point.
{
	float x; // 0x0000
	float y; // 0x0004
	float z; // 0x0008
};

struct QAngle // Implement the proper class of this at some point.
{
	float pitch; //0x0000
	float yaw; // 0x0004
	float roll; // 0x0008
};

class CHostState
{
public:
	__int32 m_iCurrentState; //0x0000
	__int32 m_iNextState; //0x0004
	Vector3 m_vecLocation; //0x0008
	QAngle m_angLocation; //0x0014
	char m_levelName[64]; //0x0020
	char m_mapGroupName[256]; //0x0060
	char m_landMarkName[256]; //0x0160
	float m_flShortFrameTime; //0x0260
	bool m_bActiveGame; //0x0264
	bool m_bRememberLocation; //0x0265
	bool m_bBackgroundLevel; //0x0266
	bool m_bWaitingForConnection; //0x0267
	bool m_bSplitScreenConnect; //0x0268
	bool m_bGameHasShutDownAndFlushedMemory; //0x0269
	bool m_bWorkshopMapDownloadPending; //0x026A
};

class CHLClient
{
public:
	void FrameStageNotify(ClientFrameStage_t curStage) // @0x1405C0740 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void(__thiscall*)(CHLClient*, ClientFrameStage_t);
		(*reinterpret_cast<OriginalFn**>(this))[58](this, curStage); /* 48 83 EC 28 89 15 ? ? ? ? */
	}
};

class ConCommandBase
{
public:
	void* m_pConCommandBaseVTable; //0x0000
	ConCommandBase* m_pNext; //0x0008
	bool m_bRegistered; //0x0010
private:
	char pad_0011[7]; //0x0011
public:
	const char* m_pszName; //0x0018
	const char* m_pszHelpString; //0x0020
private:
	char pad_0028[16]; //0x0028
public:
	__int32 m_nFlags; //0x0038
private:
	char pad_003C[4]; //0x003C
}; //Size: 0x0038

class ConVar
{
public:
	ConCommandBase m_ConCommandBase; // 0x0000
	void* m_pConVarVTable; //0x0040
	ConVar* m_pParent; //0x0048
	const char* n_pszDefaultValue; //0x0050
	const char* m_pzsCurrentValue; //0x0058
	__int64 m_iStringLength; //0x0060
	float m_flValue; //0x0068
	__int64 m_iValue; //0x006C
	bool m_bHasMin; //0x0070
private:
	char pad_0071[3]; //0x0071
public:
	float m_flMinValue; //0x0074
	bool m_bHasMax; //0x0078
private:
	char pad_0079[3]; //0x0079
public:
	float m_flMaxValue; //0x007C
}; //Size: 0x0080

class CCVarIteratorInternal // Fully reversed table, just look at the virtual function table and rename the function.
{
public:
	virtual void		    SetFirst(void) = 0; //0
	virtual void		    Next(void)     = 0; //1
	virtual	bool		    IsValid(void)  = 0; //2
	virtual ConCommandBase* Get(void)      = 0; //3
};

class CCVar
{
public:
	ConCommandBase* FindCommandBase(const char* szCommandName) // @0x1405983A0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = ConCommandBase*(__thiscall*)(CCVar*, const char*);
		return (*reinterpret_cast<OriginalFn**>(this))[14](this, szCommandName);
	}

	ConVar* FindVar(const char* szVarName) // @0x1405983B0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = ConVar*(__thiscall*)(CCVar*, const char*);
		return (*reinterpret_cast<OriginalFn**>(this))[16](this, szVarName);
	}

	void* /*Implement ConCommand class.*/ FindCommand(const char* szCommandName) // @0x1405983F0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = void*(__thiscall*)(CCVar*, const char*);
		return (*reinterpret_cast<OriginalFn**>(this))[18](this, szCommandName);
	}

	CCVarIteratorInternal* FactoryInternalIterator() // @0x140597C10 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		using OriginalFn = CCVarIteratorInternal*(__thiscall*)(CCVar*);
		return (*reinterpret_cast<OriginalFn**>(this))[41](this);
	}

	std::unordered_map<std::string, ConCommandBase*> DumpToMap()
	{
		std::stringstream ss;
		CCVarIteratorInternal* itint = FactoryInternalIterator(); // Allocatd new InternalIterator.

		std::unordered_map<std::string, ConCommandBase*> allConVars;

		for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
		{
			ConCommandBase* command = itint->Get();
			const char* commandName = command->m_pszName;
			allConVars[commandName] = command;
		}

		return allConVars;
	}
};

struct Interface
{
	__int64 (*InterfacePtr)(void);
	const char* InterfaceName;
	__int64* NextInterfacePtr;
};

/////////////////////////////////////////////////////////////////////////////
// Initialize Game Globals

namespace GameGlobals
{
	extern CHostState* HostState;
	extern CInputSystem* InputSystem;
	extern CCVar* Cvar;

	void InitGameGlobals();
	extern bool IsInitialized;
}