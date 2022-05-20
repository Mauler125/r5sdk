//============ Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable memory class.
//===========================================================================//
#pragma once

template <class T>
class CUtlMemory
{
public:
	CUtlMemory() {};
	CUtlMemory<T>(uintptr_t ptr) : m_pMemory(ptr) {};

private:
	void* m_pMemory;
	int64_t m_nAllocationCount;
	int64_t m_nGrowSize;
};

