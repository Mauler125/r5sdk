//============ Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable memory class.
//===========================================================================//

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

struct __declspec(align(8)) CUtlMemory
{
	void* m_pMemory;
	int64_t m_nAllocationCount;
	int64_t m_nGrowSize;
};

#endif // UTLMEMORY_H
