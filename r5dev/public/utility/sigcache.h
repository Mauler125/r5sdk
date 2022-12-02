#ifndef SIGCACHE_H
#define SIGCACHE_H

#include "protoc/sig_map.pb.h"

#define SIGDB_MAGIC	(('p'<<24)+('a'<<16)+('M'<<8)+'S')
#define SIGDB_VERSION 0x1

#ifdef DEDICATED
#define SIGDB_FILE "cfg\\server\\startup.bin"
#else
#define SIGDB_FILE "cfg\\client\\startup.bin"
#endif

class CSigCache
{
public:
	void AddEntry(const string& svPattern, const uint64_t nRVA);
	bool FindEntry(const string& svPattern, uint64_t& nRVA) const;

	bool LoadCache(const string& svCacheFile);
	bool WriteCache(const string& svCacheFile);

	SigMap_Pb m_Cache;
	bool m_bInitialized;
};

struct SigDBHeader_t
{
	int m_nMagic;
	int m_nVersion;
	FILETIME m_FileTime;
};

#endif // !SIGCACHE_H
