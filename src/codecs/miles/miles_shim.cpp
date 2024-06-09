//=============================================================================//
// 
// Purpose: Miles Sound System interface shim
// 
//-----------------------------------------------------------------------------
// The engine is compiled with version 10.0.42, this shim layer fixes any
// incompatibilities between upgrades. On more recent versions of the Miles
// Sound System, some exports have been renamed and/or thoroughly changed.
// If we upgrade to these versions, we need to convert this into an actual
// DLL shim layer instead of linking it statically with the SDK module.
//=============================================================================//
#include "miles_impl.h"
#include "miles_shim.h"

unsigned int MilesSampleSetSourceRaw(__int64 a1, __int64 a2, unsigned int a3, int a4, unsigned __int16 a5, bool a6)
{
	// interface fix from 10.0.42 --> 10.0.47. As of version (10.0.43 ?) the
	// export 'MilesSampleSetSourceRaw' has a newly added bool parameter. The
	// purpose of this is unknown, but we need to set it to false as they
	// otherwise would distort the voice comm bus.
	return v_MilesSampleSetSourceRaw(a1, a2, a3, a4, a5, false);
}

void MilesShim::Detour(const bool bAttach) const
{
	DetourSetup(&v_MilesSampleSetSourceRaw, &MilesSampleSetSourceRaw, bAttach);
}
