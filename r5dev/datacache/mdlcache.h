#ifndef MDLCACHE_H
#define MDLCACHE_H
#include "public/include/studio.h"
#include "datacache/imdlcache.h"

struct CMDLFallBack
{
	studiohdr_t* m_pErrorHDR{};
	MDLHandle_t m_hErrorMDL{};
	studiohdr_t* m_pEmptyHDR{};
	MDLHandle_t m_hEmptyMDL{};

	// This has to be cleared if 'common.rpak' is getting unloaded!
	void Clear(void)
	{
		m_pEmptyHDR = nullptr;
		m_hErrorMDL = NULL;
		m_pEmptyHDR = nullptr;
		m_hEmptyMDL = NULL;
	}
};
inline CMDLFallBack* g_pMDLFallback = new CMDLFallBack();

class CMDLCache
{
public:
	static studiohdr_t* FindMDL(CMDLCache* pMDLCache, MDLHandle_t handle, void* a3);
	static void FindCachedMDL(CMDLCache* pMDLCache, void* a2, void* a3);
	static studiohdr_t* FindUncachedMDL(CMDLCache* pMDLCache, MDLHandle_t handle, void* a3, void* a4);
	static studiohdr_t* GetStudioHdr(CMDLCache* pMDLCache, MDLHandle_t handle);

	CMDLCache* m_pVTable;
	void* m_pStrCmp;             // string compare func;
	void* m_pModelCacheSection;  // IDataCacheSection*
	int m_nModelCacheFrameLocks; //
	// TODO..
};

extern studiohdr_t* pErrorStudioHDR;
extern MDLHandle_t hErrorMDL;

inline CMemory p_CMDLCache__FindMDL;
inline auto v_CMDLCache__FindMDL = p_CMDLCache__FindMDL.RCast<studiohdr_t* (*)(CMDLCache* pCache, void* a2, void* a3)>();

inline CMemory p_CMDLCache__FindCachedMDL;
inline auto v_CMDLCache__FindCachedMDL = p_CMDLCache__FindCachedMDL.RCast<void(*)(CMDLCache* pCache, void* a2, void* a3)>();

inline CMemory p_CMDLCache__FindUncachedMDL;
inline auto v_CMDLCache__FindUncachedMDL = p_CMDLCache__FindUncachedMDL.RCast<studiohdr_t* (*)(CMDLCache* pCache, MDLHandle_t handle, void* a3, void* a4)>();

inline CMemory p_CMDLCache__GetStudioHdr;
inline auto v_CMDLCache__GetStudioHdr = p_CMDLCache__GetStudioHdr.RCast<studiohdr_t* (*)(CMDLCache* pCache, MDLHandle_t handle)>();

inline CMemory m_MDLDict;
inline LPCRITICAL_SECTION* m_MDLMutex = nullptr;
inline CMDLCache* g_MDLCache = nullptr;


void MDLCache_Attach();
void MDLCache_Detach();
///////////////////////////////////////////////////////////////////////////////
class HMDLCache : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CMDLCache::FindMDL                   : 0x" << std::hex << std::uppercase << p_CMDLCache__FindMDL.GetPtr()         << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CMDLCache::FindCachedMDL             : 0x" << std::hex << std::uppercase << p_CMDLCache__FindCachedMDL.GetPtr()   << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CMDLCache::FindUncachedMDL           : 0x" << std::hex << std::uppercase << p_CMDLCache__FindUncachedMDL.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CMDLCache::GetStudioHdr              : 0x" << std::hex << std::uppercase << p_CMDLCache__GetStudioHdr.GetPtr()    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: m_MDLMutex                           : 0x" << std::hex << std::uppercase << m_MDLMutex                            << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: m_MDLDict                            : 0x" << std::hex << std::uppercase << m_MDLDict.GetPtr()                    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_MDLCache                           : 0x" << std::hex << std::uppercase << g_MDLCache                            << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_CMDLCache__FindMDL = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF1\x0F\xB7\xEA"), "xxxx?xxxx?xxxx?xxxxxxxxxxx");
		v_CMDLCache__FindMDL = p_CMDLCache__FindMDL.RCast<studiohdr_t* (*)(CMDLCache*, void*, void*)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F1 0F B7 EA*/

		p_CMDLCache__FindCachedMDL = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4D\x85\xC0\x74\x7A\x48\x89\x6C\x24\x00"), "xxxxxxxxx?");
		v_CMDLCache__FindCachedMDL = p_CMDLCache__FindCachedMDL.RCast<void(*)(CMDLCache*, void*, void*)>(); /*4D 85 C0 74 7A 48 89 6C 24 ?*/

		p_CMDLCache__FindUncachedMDL = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x20\x48\x8B\xE9\x0F\xB7\xFA"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxx");
		v_CMDLCache__FindUncachedMDL = p_CMDLCache__FindUncachedMDL.RCast<studiohdr_t* (*)(CMDLCache*, MDLHandle_t , void*, void*)>();

		p_CMDLCache__GetStudioHdr = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8D\x0D\x00\x00\x00\x00\x0F\xB7\xDA\xFF\x15\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x14\x5B\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\x5C\xD0\x00\xFF\x15\x00\x00\x00\x00\x48\x8B\x03\x48\x8B\x48\x08"), "xxxxxxxxx????xxxxx????xxx????xxxxxxx????xxxx?xx????xxxxxxx");;
		v_CMDLCache__GetStudioHdr = p_CMDLCache__GetStudioHdr.RCast<studiohdr_t* (*)(CMDLCache* pCache, MDLHandle_t handle)>();
	}
	virtual void GetVar(void) const
	{
		m_MDLMutex = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\xBA\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x0F\xB6\x05\x00\x00\x00\x00"), "xxxxx????xxx????xx????xxx????")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<LPCRITICAL_SECTION*>();

		m_MDLDict = p_CMDLCache__FindMDL.FindPattern("48 8B 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7);

		g_MDLCache = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x44\x0F\xB7\x82\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\xA0\x30\x01\x00\x00"), "xxx????xxxx????xxxxxxxxxx")
			.ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMDLCache*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMDLCache);
#endif // MDLCACHE_H
