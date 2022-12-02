#ifndef SIGCACHE_H
#define SIGCACHE_H

#include "protoc/sig_map.pb.h"

#define SIGDB_MAGIC	(('p'<<24)+('a'<<16)+('M'<<8)+'S')
#define SIGDB_DICT_SIZE 20

#define SIGDB_MAJOR_VERSION 0x1 // Increment when library changes are made.
#define SIGDB_MINOR_VERSION 0x1 // Increment when SDK updates are released.

#ifdef DEDICATED
#define SIGDB_FILE "cfg\\server\\startup.bin"
#else
#define SIGDB_FILE "cfg\\client\\startup.bin"
#endif

class CSigCache
{
public:
	CSigCache() { m_bInitialized = false; };
	~CSigCache() {};

	void AddEntry(const string& svPattern, const uint64_t nRVA);
	bool FindEntry(const string& svPattern, uint64_t& nRVA) const;

	bool LoadCache(const string& svCacheFile);
	bool WriteCache(const string& svCacheFile);

private:
	bool CompressBlob(size_t nSrcLen, size_t& nDstSize, uint32_t& nCrc32, const uint8_t* pSrcBuf, uint8_t* pDstBuf) const;
	bool DecompressBlob(size_t nSrcLen, size_t& nDstSize, uint32_t& nCrc32, const uint8_t* pSrcBuf, uint8_t* pDstBuf) const;

	SigMap_Pb m_Cache;
	bool m_bInitialized;
};

#pragma pack(push, 1)
struct SigDBHeader_t
{
	int m_nMagic;
	uint16_t m_nMajorVersion;
	uint16_t m_nMinorVersion;
	uint64_t m_nBlobSizeMem;
	uint64_t m_nBlobSizeDisk;
	uint32_t m_nBlobHash;
};
#pragma pack(pop)

#endif // !SIGCACHE_H
