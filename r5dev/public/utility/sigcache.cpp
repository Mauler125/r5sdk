//===========================================================================//
//
// Purpose: Implementation of the CSigCache class.
//
//===========================================================================//
#include "core/stdafx.h"
#include "public/utility/binstream.h"
#include "sigcache.h"

void CSigCache::AddEntry(const string& svPattern, const uint64_t nRVA)
{
	if (g_SigCache.m_bUseCache)
	{
		(*g_SigCache.m_Cache.mutable_smap())[svPattern] = nRVA;
	}
}

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