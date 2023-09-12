//===========================================================================//
//
// Purpose: NVIDIA Reflex utilities
//
//===========================================================================//
#include "reflex.h"
#include "mathlib/mathlib.h"

#define GFX_NVN_MAX_FRAME_COUNT 4000

// Static frame number counter for latency markers.
int s_ReflexFrameNumber = -1;

//-----------------------------------------------------------------------------
// Purpose: gets the reflex frame number
// Output : int
//-----------------------------------------------------------------------------
int GFX_GetFrameNumber(void)
{
	return s_ReflexFrameNumber;
}

//-----------------------------------------------------------------------------
// Purpose: increments the reflex frame number
//-----------------------------------------------------------------------------
void GFX_IncrementFrameNumber(void)
{
	// Start over again to make sure this never overflows. Each frame number
	// within the last 1000 must be unique, so its safe to reset after 4000.
	if (++s_ReflexFrameNumber >= GFX_NVN_MAX_FRAME_COUNT)
		s_ReflexFrameNumber = 0;
}

//-----------------------------------------------------------------------------
// Purpose: runs a frame of the low latency sdk
// Input  : *device              - 
//          useLowLatencyMode    - 
//          useLowLatencyBoost   - 
//          useMarkersToOptimize - 
//          maxFramesPerSecond   - 
//-----------------------------------------------------------------------------
void GFX_RunLowLatencyFrame(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const bool useMarkersToOptimize,
	const float maxFramesPerSecond)
{
	Assert(device);
	Assert(IsFinite(maxFramesPerSecond));

	GFX_IncrementFrameNumber();

	NV_SET_SLEEP_MODE_PARAMS params = {};
	params.version = NV_SET_SLEEP_MODE_PARAMS_VER1;

	params.bLowLatencyMode = useLowLatencyMode;
	params.bLowLatencyBoost = useLowLatencyMode && useLowLatencyBoost;
	params.minimumIntervalUs = maxFramesPerSecond > 0
		? (NvU32)((1000.0f / maxFramesPerSecond) * 1000.0f)
		: 0;
	params.bUseMarkersToOptimize = useMarkersToOptimize;

	NvAPI_Status status = NvAPI_D3D_SetSleepMode(device, &params);

	if (status == NVAPI_OK)
		NvAPI_D3D_Sleep(device);
}

//-----------------------------------------------------------------------------
// Purpose: sets the latency marker
// Input  : *device        - 
//          frameNumber    - 
//          markerType     - 
//-----------------------------------------------------------------------------
void GFX_SetLatencyMarker(IUnknown* device,
	const NV_LATENCY_MARKER_TYPE markerType)
{
	NV_LATENCY_MARKER_PARAMS params = {};
	params.version = NV_LATENCY_MARKER_PARAMS_VER1;
	params.frameID = s_ReflexFrameNumber;
	params.markerType = markerType;

	NvAPI_D3D_SetLatencyMarker(device, &params);
}
