//===========================================================================//
//
// Purpose: Implementation of the CSigCache class.
//
//===========================================================================//
#include "core/stdafx.h"
#include "public/utility/binstream.h"
#include "sigcache.h"

//-----------------------------------------------------------------------------
// Purpose: creates a pair of a pattern (key) and relative virtual address (value)
// Input  : &svPattern - 
//			nRVA - 
//-----------------------------------------------------------------------------
void CSigCache::AddEntry(const string& svPattern, const uint64_t nRVA)
{
	if (g_SigCache.m_bUseCache)
	{
		(*g_SigCache.m_Cache.mutable_smap())[svPattern] = nRVA;
	}
}

//-----------------------------------------------------------------------------
// Purpose: finds a pattern key in the cache map and sets its value to nRVA
// Input  : &svPattern - 
//			&nRVA - 
// Output : true if key is found, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::FindEntry(const string& svPattern, uint64_t& nRVA) const
{
	if (g_SigCache.m_bInitialized)
	{
		google::protobuf::Map sMap = g_SigCache.m_Cache.smap();
		auto p = sMap.find(svPattern);

		if (p != sMap.end())
		{
			nRVA = p->second;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: writes the cache map to the disk
//-----------------------------------------------------------------------------
void CSigCache::WriteCache()
{
	CIOStream writer("bin\\startup.smap", CIOStream::Mode_t::WRITE);

	if (!writer.IsWritable())
	{
		// Error message..
		return;
	}

	SigDBHeader_t header;

	header.m_nMagic = SIGDB_MAGIC;
	header.m_nVersion = SIGDB_VERSION;
	GetSystemTimeAsFileTime(&header.m_FileTime);
	const string svBuffer = m_Cache.SerializeAsString();

	writer.Write(header);
	writer.Write(svBuffer.data(), svBuffer.size());
}