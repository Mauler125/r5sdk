//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "concommandhash.h"

//-----------------------------------------------------------------------------
// Console command hash data structure
//-----------------------------------------------------------------------------
CConCommandHash::CConCommandHash()
{
	Purge(true);
}

CConCommandHash::~CConCommandHash()
{
	Purge(false);
}

void CConCommandHash::Purge(bool bReinitialize)
{
	m_aBuckets.Purge();
	m_aDataPool.Purge();
	if (bReinitialize)
	{
		Init();
	}
}

// Initialize.
void CConCommandHash::Init(void)
{
	// kNUM_BUCKETS must be a power of two.
	COMPILE_TIME_ASSERT((kNUM_BUCKETS & (kNUM_BUCKETS - 1)) == 0);

	// Set the bucket size.
	m_aBuckets.SetSize(kNUM_BUCKETS);
	for (int iBucket = 0; iBucket < kNUM_BUCKETS; ++iBucket)
	{
		m_aBuckets[iBucket] = m_aDataPool.InvalidIndex();
	}

	// Calculate the grow size.
	int nGrowSize = 4 * kNUM_BUCKETS;
	m_aDataPool.SetGrowSize(nGrowSize);
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int), 
//			WITH a check to see if the element already exists within the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Insert(ConCommandBase* cmd)
{
	// Check to see if that key already exists in the buckets (should be unique).
	CCommandHashHandle_t hHash = Find(cmd);
	if (hHash != InvalidHandle())
		return hHash;

	return FastInsert(cmd);
}
//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int),
//          WITHOUT a check to see if the element already exists within the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::FastInsert(ConCommandBase* cmd)
{
	// Get a new element from the pool.
	intptr_t iHashData = m_aDataPool.Alloc(true);
	HashEntry_t* RESTRICT pHashData = &m_aDataPool[iHashData];
	if (!pHashData)
		return InvalidHandle();

	HashKey_t key = Hash(cmd);

	// Add data to new element.
	pHashData->m_uiKey = key;
	pHashData->m_Data = cmd;

	// Link element.
	int iBucket = key & kBUCKETMASK; // HashFuncs::Hash( uiKey, m_uiBucketMask );
	m_aDataPool.LinkBefore(m_aBuckets[iBucket], iHashData);
	m_aBuckets[iBucket] = iHashData;

	return iHashData;
}

//-----------------------------------------------------------------------------
// Purpose: Remove a given element from the hash.
//-----------------------------------------------------------------------------
void CConCommandHash::Remove(CCommandHashHandle_t hHash) /*RESTRICT*/
{
	HashEntry_t* /*RESTRICT*/ entry = &m_aDataPool[hHash];
	HashKey_t iBucket = entry->m_uiKey & kBUCKETMASK;
	if (m_aBuckets[iBucket] == hHash)
	{
		// It is a bucket head.
		m_aBuckets[iBucket] = m_aDataPool.Next(hHash);
	}
	else
	{
		// Not a bucket head.
		m_aDataPool.Unlink(hHash);
	}

	// Remove the element.
	m_aDataPool.Remove(hHash);
}

//-----------------------------------------------------------------------------
// Purpose: Remove all elements from the hash
//-----------------------------------------------------------------------------
void CConCommandHash::RemoveAll(void)
{
	m_aBuckets.RemoveAll();
	m_aDataPool.RemoveAll();
}

//-----------------------------------------------------------------------------
// Find hash entry corresponding to a string name
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(
	const char* name, HashKey_t hashkey) const /*RESTRICT*/
{
	// hash the "key" - get the correct hash table "bucket"
	int iBucket = hashkey & kBUCKETMASK;

	for (datapool_t::IndexLocalType_t iElement = m_aBuckets[iBucket];
		iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next(iElement))
	{
		const HashEntry_t& element = m_aDataPool[iElement];
		if (element.m_uiKey == hashkey && // if hashes of strings match,
			Q_stricmp(name, element.m_Data->GetName()) == 0) // then test the actual strings
		{
			return iElement;
		}
	}

	// found nuffink
	return InvalidHandle();
}

//-----------------------------------------------------------------------------
// Find a command in the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(const ConCommandBase* cmd) const /*RESTRICT*/
{
	// Set this #if to 1 if the assert at bottom starts whining --
	// that indicates that a console command is being double-registered,
	// or something similarly non-fatally bad. With this #if 1, we'll search
	// by name instead of by pointer, which is more robust in the face
	// of double registered commands, but obviously slower.
#if 0 
	return Find(cmd->GetName());
#else
	HashKey_t hashkey = Hash(cmd);
	int iBucket = hashkey & kBUCKETMASK;

	// hunt through all entries in that bucket
	for (datapool_t::IndexLocalType_t iElement = m_aBuckets[iBucket];
		iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next(iElement))
	{
		const HashEntry_t& element = m_aDataPool[iElement];
		if (element.m_uiKey == hashkey && // if the hashes match... 
			element.m_Data == cmd) // and the pointers...
		{
			// in debug, test to make sure we don't have commands under the same name
			// or something goofy like that
			Assert(iElement == Find(cmd->GetName()),
				"ConCommand %s had two entries in the hash!", cmd->GetName());

			// return this element
			return iElement;
		}
	}

	// found nothing.
#ifdef DBGFLAG_ASSERT // double check against search by name
	CCommandHashHandle_t dbghand = Find(cmd->GetName());

	Assert(InvalidHandle() == dbghand,
		"ConCommand %s couldn't be found by pointer, but was found by name!", cmd->GetName());
#endif
	return InvalidHandle();
#endif
}


//#ifdef _DEBUG
// Dump a report to MSG
void CConCommandHash::Report(void)
{
	Msg(eDLL_T::COMMON, "Console command hash bucket load:\n");
	int total = 0;
	for (int iBucket = 0; iBucket < kNUM_BUCKETS; ++iBucket)
	{
		int count = 0;
		CCommandHashHandle_t iElement = m_aBuckets[iBucket]; // get the head of the bucket
		while (iElement != m_aDataPool.InvalidIndex())
		{
			++count;
			iElement = m_aDataPool.Next(iElement);
		}

		Msg(eDLL_T::COMMON, "%d: %d\n", iBucket, count);
		total += count;
	}

	Msg(eDLL_T::COMMON, "\tAverage: %.1f\n", total / ((float)(kNUM_BUCKETS)));
}
//#endif