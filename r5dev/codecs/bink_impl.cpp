#include "core/stdafx.h"
#include "codecs/bink_impl.h"

void* BinkOpen(HANDLE hBinkFile, UINT32 nFlags)
{
	void* pHandle = v_BinkOpen(hBinkFile, nFlags);
	if (!pHandle)
	{
		// Retrieve BinkOpen error using the DLL's exported function "BinkGetError()".
		Error(eDLL_T::VIDEO, NO_ERROR, "%s: %s\n", __FUNCTION__, v_BinkGetError());
	}

	return pHandle;
}

void BinkImpl_Attach()
{
	DetourAttach(&v_BinkOpen, &BinkOpen);
}

void BinkImpl_Detach()
{
	DetourDetach(&v_BinkOpen, &BinkOpen);
}