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
	Assert(!m_bInitialized);
	(*m_Cache.mutable_smap())[svPattern] = nRVA;
}

//-----------------------------------------------------------------------------
// Purpose: finds a pattern key in the cache map and sets its value to nRVA
// Input  : &svPattern - 
//			&nRVA - 
// Output : true if key is found, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::FindEntry(const string& svPattern, uint64_t& nRVA) const
{
	if (m_bInitialized)
	{
		google::protobuf::Map sMap = m_Cache.smap();
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
// Purpose: loads the cache map from the disk
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::LoadCache(const string& svCacheFile)
{
	Assert(!m_bInitialized); // Recursive load.
	CIOStream reader(svCacheFile, CIOStream::Mode_t::READ);

	if (!reader.IsReadable())
	{
		return false;
	}
	if (!reader.GetSize() > sizeof(SigDBHeader_t))
	{
		return false;
	}

	SigDBHeader_t sigDbHeader;
	sigDbHeader.m_nMagic = reader.Read<int>();

	if (sigDbHeader.m_nMagic != SIGDB_MAGIC)
	{
		return false;
	}

	sigDbHeader.m_nVersion = reader.Read<int>();
	if (sigDbHeader.m_nVersion != SIGDB_VERSION)
	{
		return false;
	}

	sigDbHeader.m_FileTime = reader.Read<FILETIME>();

	vector<uint8_t> vData;
	size_t nSize = (static_cast<size_t>(reader.GetSize()) - sizeof(SigDBHeader_t));

	vData.resize(nSize);
	uint8_t* pBuf = vData.data();
	reader.Read<uint8_t>(*pBuf, nSize);

	if (!m_Cache.ParseFromArray(pBuf, nSize))
	{
		return false;
	}

	m_bInitialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: writes the cache map to the disk
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CSigCache::WriteCache(const string& svCacheFile)
{
	if (m_bInitialized)
	{
		// Only write when we don't have anything valid on the disk.
		return false;
	}

	CIOStream writer(svCacheFile, CIOStream::Mode_t::WRITE);
	if (!writer.IsWritable())
	{
		// Error message..
		return false;
	}

	SigDBHeader_t header;

	header.m_nMagic = SIGDB_MAGIC;
	header.m_nVersion = SIGDB_VERSION;
	GetSystemTimeAsFileTime(&header.m_FileTime);
	const string svBuffer = m_Cache.SerializeAsString();

	writer.Write(header);
	writer.Write(svBuffer.data(), svBuffer.size());

	return true;
}
