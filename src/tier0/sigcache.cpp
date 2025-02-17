//===========================================================================//
//
// Purpose: Implementation of the CSigCache class.
// 
//===========================================================================//
// sigcache.cpp
// 
// The system creates a static cache file on the disk, who's blob contains a 
// map of string signatures and its precomputed relative virtual address.
// 
// This file gets loaded and parsed during DLL init. If the file is absent or 
// outdated/corrupt, the system will generate a new cache file if enabled.
// 
// By caching the relative virtual addresses, we can drop a significant amount 
// of time initializing the DLL by parsing the precomputed data instead of 
// searching for each signature in the memory region of the target executable.
//
///////////////////////////////////////////////////////////////////////////////
#include "tier0/sigcache.h"

//-----------------------------------------------------------------------------
// Purpose: whether or not to disable the caching of signatures
// Input  : bDisabled - (true = disabled)
//-----------------------------------------------------------------------------
void CSigCache::SetDisabled(const bool bDisabled)
{
	m_bDisabled = bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose: clears the signature cache memory
//-----------------------------------------------------------------------------
void CSigCache::InvalidateMap()
{
	if (m_bDisabled)
	{
		return;
	}

	m_Cache.mutable_smap()->clear();
}

//-----------------------------------------------------------------------------
// Purpose: creates a map of a pattern and relative virtual address
// Input  : *pPattern   - (key)
//          nPatternLen - 
//          nRVA        - (value)
//-----------------------------------------------------------------------------
void CSigCache::AddEntry(const void* pPattern, const size_t nPatternLen, const u64 nRVA)
{
	if (m_bDisabled)
	{
		return;
	}

	const u64 hash = MurmurHash64(pPattern, nPatternLen, SIGDB_MURMUR_SEED);
	(*m_Cache.mutable_smap())[hash] = nRVA;
}

//-----------------------------------------------------------------------------
// Purpose: finds a pattern key in the cache map and sets its value to nRVA
// Input  : *pPattern   - (key)
//          nPatternLen - 
//          &nRVA       - (value)
// Output : true if key is found, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::FindEntry(const void* pPattern, const size_t nPatternLen, u64& nRVA)
{
	if (!m_bDisabled && m_bInitialized)
	{
		const u64 hash = MurmurHash64(pPattern, nPatternLen, SIGDB_MURMUR_SEED);
		google::protobuf::Map< u64, u64 >* const sMap = m_Cache.mutable_smap();

		const auto p = sMap->find(hash);

		if (p != sMap->end())
		{
			nRVA = p->second;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: loads the cache map from the disk
// Input  : *szCacheFile - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::ReadCache(const char* const szCacheFile)
{
	Assert(!m_bInitialized); // Recursive load.

	if (m_bDisabled)
	{
		return false;
	}

	std::ifstream reader(szCacheFile, std::ios::in | std::ios::binary);

	if (!reader)
	{
		return false;
	}

	SigDBHeader_s header;
	reader.read((char*)&header, sizeof(SigDBHeader_s));

	if (reader.eof())
	{
		Error(eDLL_T::COMMON, NO_ERROR, "%s - Cache map '%s' appears truncated\n", __FUNCTION__, szCacheFile);
		return false;
	}

	if (header.magic != SIGDB_MAGIC ||
		header.majorVersion != SIGDB_MAJOR_VERSION ||
		header.minorVersion != SIGDB_MINOR_VERSION)
	{
		return false;
	}

	if (!m_Cache.ParseFromIstream(&reader))
	{
		Error(eDLL_T::COMMON, NO_ERROR, "%s - Stream deserialization failed for '%s'\n", __FUNCTION__, szCacheFile);
		return false;
	}

	m_bInitialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: writes the cache map to the disk
// Input  : *szCacheFile - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::WriteCache(const char* const szCacheFile) const
{
	if (m_bDisabled || m_bInitialized)
	{
		// Only write when we don't have anything valid on the disk.
		return false;
	}

	std::ofstream writer(szCacheFile, std::ios::out | std::ios::binary);

	if (!writer)
	{
		Error(eDLL_T::COMMON, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, szCacheFile);
		return false;
	}

	SigDBHeader_s header;

	header.magic = -1; // Magic is only written if serialization succeeded.
	header.majorVersion = SIGDB_MAJOR_VERSION;
	header.minorVersion = SIGDB_MINOR_VERSION;
	header.blobSize = m_Cache.ByteSizeLong();

	writer.write((const char*)&header, sizeof(SigDBHeader_s));
	
	if (!m_Cache.SerializeToOstream(&writer))
	{
		Error(eDLL_T::COMMON, NO_ERROR, "%s - Stream serialization failed for '%s'\n", __FUNCTION__, szCacheFile);
		return false;
	}

	writer.seekp(0);

	header.magic = SIGDB_MAGIC;
	writer.write((const char*)&header.magic, sizeof(header.magic));

	return true;
}

//-----------------------------------------------------------------------------
// Singleton signature cache
//-----------------------------------------------------------------------------
CSigCache g_SigCache;
