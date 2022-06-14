//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef MEMPOOL_H
#define MEMPOOL_H

struct CUtlMemoryPool
{
	class CBlob
	{
	public:
		short m_nAlignment; // to int align the struct.
		short m_NumBlobs;   // Number of blobs.
		const char* m_pszAllocOwner;
		CBlob* m_pPrev, * m_pNext;
	};

	int m_BlockSize;
	int m_BlocksPerBlob;
	int m_GrowMode;
	int m_BlocksAllocated;
	CBlob m_BlobHead;
};

#endif // MEMPOOL_H