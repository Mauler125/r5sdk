#include "core/stdafx.h"
#include "tier0/tslist.h"

void* MemAlloc_Internal(void* pPool, int64_t nSize)
{
	return v_MemAlloc_Internal(pPool, nSize);
}

void TSList_Attach()
{
	DetourAttach((LPVOID*)&v_MemAlloc_Internal, &MemAlloc_Internal);
}

void TSList_Detach()
{
	DetourDetach((LPVOID*)&v_MemAlloc_Internal, &MemAlloc_Internal);
}