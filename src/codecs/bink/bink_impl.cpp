#include "core/stdafx.h"
#include "codecs/bink/bink_impl.h"

//-----------------------------------------------------------------------------
// Purpose: opens bik video by handle, and logs any error caused during loading
// Input  : hBinkFile - 
//          nFlags - 
// Output : pointer to bik video structure, null if failed
//-----------------------------------------------------------------------------
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

///////////////////////////////////////////////////////////////////////////////
void BinkCore::Detour(const bool bAttach) const
{
	DetourSetup(&v_BinkOpen, &BinkOpen, bAttach);
}
