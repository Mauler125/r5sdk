//===========================================================================//
//
// Purpose: NVIDIA Reflex utilities
//
//===========================================================================//
#include "reflex.h"
#include "mathlib/mathlib.h"


//-----------------------------------------------------------------------------
// Purpose: runs the low latency sdk
// Input  : device             - 
//          useLowLatencyMode  - 
//          useLowLatencyBoost - 
//          maxFramesPerSecond -
//-----------------------------------------------------------------------------
void GFX_RunLowLatencySDK(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const float maxFramesPerSecond)
{
	Assert(device);
	Assert(IsFinite(maxFramesPerSecond));

	NV_SET_SLEEP_MODE_PARAMS params = {};
	params.version = NV_SET_SLEEP_MODE_PARAMS_VER1;

	params.bLowLatencyMode = useLowLatencyMode;
	params.bLowLatencyBoost = useLowLatencyMode && useLowLatencyBoost;
	params.minimumIntervalUs = maxFramesPerSecond > 0
		? (NvU32)((1000.0f / maxFramesPerSecond) * 1000.0f)
		: 0;
	params.bUseMarkersToOptimize = false;

	NvAPI_Status status = NvAPI_D3D_SetSleepMode(device, &params);

	if (status == NVAPI_OK)
		NvAPI_D3D_Sleep(device);
}
