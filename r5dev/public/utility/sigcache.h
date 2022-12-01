#ifndef SIGCACHE_H
#define SIGCACHE_H

#include "protoc/sig_map.pb.h"

#define SIGDB_MAGIC	(('p'<<24)+('a'<<16)+('M'<<8)+'S')
#define SIGDB_VERSION 0x1

class CSigCache
{
public:

	// Save
	// Load
	// Clear

	void AddEntry(const string& svPattern, const uint64_t nRVA);
	void WriteCache();

	SigMap_Pb m_Cache;
	bool m_bInitialized;
	bool m_bUseCache = true;
};

struct SigDBHeader_t
{
	int m_nMagic;
	int m_nVersion;
	FILETIME m_FileTime;
};

#endif // !SIGCACHE_H
