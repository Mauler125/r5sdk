#ifndef TIER0_SIGCACHE_H
#define TIER0_SIGCACHE_H

#include "protoc/sig_map.pb.h"

#define SIGDB_MAGIC	(('p'<<24)+('a'<<16)+('M'<<8)+'S')
#define SIGDB_MURMUR_SEED 0x3F0D4710

#define SIGDB_MAJOR_VERSION 0x3 // Increment when library changes are made.
#define SIGDB_MINOR_VERSION 0x0 // Increment when SDK updates are released.

class CSigCache
{
public:
	CSigCache()
		: m_bInitialized(false)
		, m_bDisabled(false) {};
	~CSigCache() {};

	void SetDisabled(const bool bDisabled);
	void InvalidateMap();

	void AddEntry(const void* const pPattern, const size_t nPatternLen, const u64 nRVA);
	bool FindEntry(const void* const pPattern, const size_t nPatternLen, u64& nRVA);

	bool ReadCache(const char* const szCacheFile);
	bool WriteCache(const char* const szCacheFile) const;

private:
	SigMap_Pb m_Cache;
	bool m_bInitialized;
	bool m_bDisabled;
};

extern CSigCache g_SigCache;

#pragma pack(push, 1)
struct SigDBHeader_s
{
	s32 magic;
	u16 majorVersion;
	u16 minorVersion;
	u64 blobSize;
};
#pragma pack(pop)

#endif // TIER0_SIGCACHE_H
