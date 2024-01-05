#ifndef MDLCACHE_H
#define MDLCACHE_H
#include "tier0/threadtools.h"
#include "tier1/utldict.h"
#include "tier1/refcount.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "public/studio.h"
#include "public/vphysics/vcollide.h"
#include "public/rtech/ipakfile.h"

class CStudioFallbackHandler
{
public:
	CStudioFallbackHandler(void)
		: m_pFallbackHDR(nullptr)
		, m_hFallbackMDL(NULL)
	{}

	// This must be cleared if 'common.rpak' is getting unloaded!
	inline void Clear(void)
	{
		m_pFallbackHDR = nullptr;
		m_hFallbackMDL = NULL;

		m_BadMdlHandles.clear();
	}

	inline bool HasFallbackModel() const
	{
		return !!m_hFallbackMDL;
	}

	inline void SetFallbackModel(studiohdr_t* const studioHdr, const MDLHandle_t handle)
	{
		m_pFallbackHDR = studioHdr;
		m_hFallbackMDL = handle;
	}

	inline studiohdr_t* GetFallbackModelHeader() const
	{
		return m_pFallbackHDR;
	}

	inline MDLHandle_t GetFallbackModelHandle() const
	{
		return m_hFallbackMDL;
	}

	inline void EnableLegacyGatherProps()
	{
		if (!old_gather_props->GetBool())
			old_gather_props->SetValue(true);
	}

	inline void DisableLegacyGatherProps()
	{
		if (old_gather_props->GetBool())
			old_gather_props->SetValue(false);
	}

	inline bool AddBadModelHandle(const MDLHandle_t handle)
	{
		auto p = m_BadMdlHandles.insert(handle);
		return !p.second;
	}

	inline void ClearBadModelHandleCache()
	{
		m_BadMdlHandles.clear();
	}

	inline bool HasInvalidModelHandles()
	{
		return !m_BadMdlHandles.empty();
	}

	inline bool AddToSuppressionList(const MDLHandle_t handle)
	{
		auto p = m_SuppressedHandles.insert(handle);
		return p.second;
	}

	inline void ClearSuppresionList()
	{
		m_SuppressedHandles.clear();
	}

private:
	studiohdr_t* m_pFallbackHDR;
	MDLHandle_t m_hFallbackMDL;

	// Keep track of bad model handles so we don't log the
	// same one twice or more to the console and cause a 
	// significant performance impact.
	std::unordered_set<MDLHandle_t> m_BadMdlHandles;

	// Don't spam on these handles when trying to get
	// hardware data.
	std::unordered_set<MDLHandle_t> m_SuppressedHandles;
};


struct CStudioVCollide : public CRefCounted<>
{
public:
	//~CStudioVCollide() // TODO: add g_PhysicsCollision to SDK
	//{
	//	g_pPhysicsCollision->VCollideUnload(&m_vcollide);
	//}
	vcollide_t* GetVCollide()
	{
		return &m_vcollide;
	}
private:
	vcollide_t m_vcollide;
};

class CStudioPhysicsGeoms : public CRefCounted<>
{
public:
	void* GetGeometryData() { return m_pGeomDataDesc; }

private:
	// TODO: ptr to another ptr to geometry data; requires reversing.
	void* m_pGeomDataDesc;
	int unk2;
	short unk3;
	short unk4;
};

struct studiophysicsref_t
{
	inline CStudioVCollide* GetStudioVCollide() const { return m_pVCollide; }
	inline CStudioPhysicsGeoms* GetPhysicsGeoms() const { return m_pPhysicsGeoms; }

	int unk0;
	int unk1;
	int unk2;
	int unk3;
	int unk4;
	int unk5;
	CStudioVCollide* m_pVCollide;
	CStudioPhysicsGeoms* m_pPhysicsGeoms;
};

struct studiocache_t
{
	inline studiohdr_t* GetStudioHdr() const { return m_pStudioHdr; }
	inline studiophysicsref_t* GetPhysicsCache() const { return m_pPhysicsCache; }

	studiohdr_t* m_pStudioHdr;
	studiophysicsref_t* m_pPhysicsCache;
	const char* m_szPropName;
	uint8_t m_pUnknown[98];
};

struct studioanimcache_t
{
	inline studiohdr_t* GetStudioHdr() const { return m_pStudioHdr; }

	studiohdr_t* m_pStudioHdr;
	const char* m_pszRigName;
	int unk0;
	int m_nSequenceCount;
	void** m_pAnimSequences;
	// TODO reverse the rest
};

// only models with type "mod_studio" have this data
struct studiodata_t
{
	inline studiocache_t* GetStudioCache() const { return m_pStudioCache; }
	inline studioanimcache_t* GetAnimCache() const { return m_pAnimCache; }
	inline CStudioHWDataRef* GetHardwareDataRef() const { return m_pHardwareRef; }

	studiocache_t* m_pStudioCache;
	studioanimcache_t* m_pAnimCache; // !TODO: reverse struct.
	unsigned short m_nRefCount;
	unsigned short m_nFlags;
	MDLHandle_t m_Handle;
	void* Unk3; // ptr to flags and model string.
	CStudioHWDataRef* m_pHardwareRef;
	void* m_pMaterialTable; // contains a large table of CMaterialGlue objects.
	int Unk5;
	char pad[72];
	CThreadFastMutex m_Mutex;
	bool m_bSearchingModelName;
	PakHandle_t m_PakHandle;
};

extern CStudioFallbackHandler g_StudioMdlFallbackHandler;

class CMDLCache : public CTier1AppSystem<IMDLCache>
{
public:
	static studiohdr_t* FindMDL(CMDLCache* const cache, const MDLHandle_t handle, void* a3);
	static void FindCachedMDL(CMDLCache* const cache, studiodata_t* const pStudioData, void* a3);
	static studiohdr_t* FindUncachedMDL(CMDLCache* const cache, const MDLHandle_t handle, studiodata_t* const pStudioData, void* a4);

	studiocache_t* GetStudioCache(const MDLHandle_t handle);
	static vcollide_t* GetVCollide(CMDLCache* const cache, const MDLHandle_t handle);
	static void* GetPhysicsGeometry(CMDLCache* const cache, const MDLHandle_t handle);

	static studiohwdata_t* GetHardwareData(CMDLCache* const cache, const MDLHandle_t handle);

	static studiohdr_t* GetErrorModel(void);
	static const char* GetErrorModelName(void);
	static MDLHandle_t GetErrorModelHandle(void);
	static bool HasErrorModel(void);
	static bool IsKnownBadModel(const MDLHandle_t handle);


	inline studiodata_t* GetStudioData(const MDLHandle_t handle)
	{
		EnterCriticalSection(&m_MDLMutex);
		studiodata_t* const studioData = m_MDLDict.Element(handle);
		LeaveCriticalSection(&m_MDLMutex);

		return studioData;
	}

	inline const char* GetModelName(const MDLHandle_t handle)
	{
		EnterCriticalSection(&m_MDLMutex);
		const char* const modelName = m_MDLDict.GetElementName(handle);
		LeaveCriticalSection(&m_MDLMutex);

		return modelName;
	}

	inline void* GetMaterialTable(const MDLHandle_t handle)
	{
		EnterCriticalSection(&m_MDLMutex);
		studiodata_t* const studioData = m_MDLDict.Element(handle);
		LeaveCriticalSection(&m_MDLMutex);

		return &studioData->m_pMaterialTable;
	}

private:
	CUtlDict<studiodata_t*, MDLHandle_t> m_MDLDict;
	CRITICAL_SECTION m_MDLMutex;
	// !TODO: reverse the rest
};

inline studiohdr_t*(*CMDLCache__FindMDL)(CMDLCache* const pCache, const MDLHandle_t handle, void* a3);
inline void(*CMDLCache__FindCachedMDL)(CMDLCache* const pCache, studiodata_t* const pStudioData, void* a3);
inline studiohdr_t*(*CMDLCache__FindUncachedMDL)(CMDLCache* const pCache, MDLHandle_t handle, studiodata_t* const pStudioData, void* a4);

inline studiohwdata_t* (*CMDLCache__GetHardwareData)(CMDLCache* const pCache, const MDLHandle_t handle);
inline vcollide_t*(*CMDLCache__GetVCollide)(CMDLCache* const pCache, const MDLHandle_t handle);
inline void* (*CMDLCache__GetPhysicsGeometry)(CMDLCache* const pCache, const MDLHandle_t handle);

inline bool(*CMDLCache__CheckData)(void* const ref, const int64_t type); // Probably incorrect name.

inline CMDLCache* g_pMDLCache = nullptr;
inline PSRWLOCK g_pMDLLock = nullptr; // Possibly a member? research required.

///////////////////////////////////////////////////////////////////////////////
class VMDLCache : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CMDLCache::FindMDL", CMDLCache__FindMDL);
		LogFunAdr("CMDLCache::FindCachedMDL", CMDLCache__FindCachedMDL);
		LogFunAdr("CMDLCache::FindUncachedMDL", CMDLCache__FindUncachedMDL);
		LogFunAdr("CMDLCache::GetHardwareData", CMDLCache__GetHardwareData);
		LogFunAdr("CMDLCache::GetVCollide", CMDLCache__GetVCollide);
		LogFunAdr("CMDLCache::GetPhysicsGeometry", CMDLCache__GetPhysicsGeometry);
		LogFunAdr("CMDLCache::CheckData", CMDLCache__CheckData);

		LogVarAdr("g_MDLCache", g_pMDLCache);
		LogVarAdr("g_MDLLock", g_pMDLLock);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F1 0F B7 EA").GetPtr(CMDLCache__FindMDL);
		g_GameDll.FindPatternSIMD("4D 85 C0 74 7A 48 89 6C 24 ??").GetPtr(CMDLCache__FindCachedMDL);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 20 48 8B E9 0F B7 FA").GetPtr(CMDLCache__FindUncachedMDL);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 0F B7 DA FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8D 14 5B 48 8D 0D ?? ?? ?? ?? 48 8B 7C D0 ?? FF 15 ?? ?? ?? ?? 48 8B 1F").GetPtr(CMDLCache__GetHardwareData);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 0F B7 DA FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8D 14 5B 48 8D 0D ?? ?? ?? ?? 48 8B 5C D0 ?? FF 15 ?? ?? ?? ?? 48 8B 03 48 8B 48 08").GetPtr(CMDLCache__GetVCollide);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 B8 ?? ?? ?? ?? 0F B7 DA").GetPtr(CMDLCache__GetPhysicsGeometry);
		g_GameDll.FindPatternSIMD("48 83 EC 08 4C 8D 14 12").GetPtr(CMDLCache__CheckData);
	}
	virtual void GetVar(void) const
	{
		// Get MDLCache singleton from CStudioRenderContext::Connect
		g_pMDLCache = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 0F 45 C8 FF 05 ?? ?? ?? ?? 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??")
			.FindPatternSelf("48 8D 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMDLCache*>();

		g_pMDLLock = CMemory(CMDLCache__GetHardwareData).Offset(0x35).FindPatternSelf("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<PSRWLOCK>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // MDLCACHE_H
