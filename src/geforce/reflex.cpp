//===========================================================================//
//
// Purpose: NVIDIA Reflex utilities
//
//===========================================================================//
#include "reflex.h"
#include "mathlib/mathlib.h"

// If false, the system will call 'NvAPI_D3D_SetSleepMode' to update the parameters.
bool s_ReflexModeInfoUpToDate = false;

// This is 'NVAPI_OK' If the call to 'NvAPI_D3D_SetSleepMode' was successful.
// If not, the Low Latency SDK will not run.
NvAPI_Status s_ReflexModeUpdateStatus = NvAPI_Status::NVAPI_OK;

// Static frame number counter for latency markers.
NvU64 s_ReflexFrameNumber = 0;
NvU64 s_ReflexLastFrameNumber = 0;

//-----------------------------------------------------------------------------
// Purpose: mark the parameters as out-of-date; force update next frame
//-----------------------------------------------------------------------------
void GFX_MarkLowLatencyParametersOutOfDate(void)
{
	s_ReflexModeInfoUpToDate = false;
}

//-----------------------------------------------------------------------------
// Purpose: mark the parameters as up-to-date
//-----------------------------------------------------------------------------
void GFX_MarkLowLatencyParametersUpToDate(void)
{
	s_ReflexModeInfoUpToDate = true;
}

//-----------------------------------------------------------------------------
// Purpose: has the user requested any changes to the low latency parameters?
//-----------------------------------------------------------------------------
bool GFX_HasPendingLowLatencyParameterUpdates(void)
{
	return s_ReflexModeInfoUpToDate == false;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether the call to 'NvAPI_D3D_SetSleepMode' was successful
//-----------------------------------------------------------------------------
bool GFX_ParameterUpdateWasSuccessful(void)
{
	return s_ReflexModeUpdateStatus == NvAPI_Status::NVAPI_OK;
}

//-----------------------------------------------------------------------------
// Purpose: gets the reflex frame number
// Output : int
//-----------------------------------------------------------------------------
NvU64 GFX_GetFrameNumber(void)
{
	return s_ReflexFrameNumber;
}

//-----------------------------------------------------------------------------
// Purpose: increments the reflex frame number
//-----------------------------------------------------------------------------
void GFX_IncrementFrameNumber(void)
{
	++s_ReflexFrameNumber;
}

//-----------------------------------------------------------------------------
// Purpose: updates the low latency parameters
// Input  : *device              - 
//          useLowLatencyMode    - 
//          useLowLatencyBoost   - 
//          useMarkersToOptimize - 
//          maxFramesPerSecond   - 
//-----------------------------------------------------------------------------
void GFX_UpdateLowLatencyParameters(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const bool useMarkersToOptimize,
	const float maxFramesPerSecond)
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
	params.bUseMarkersToOptimize = useMarkersToOptimize;

	s_ReflexModeUpdateStatus = NvAPI_D3D_SetSleepMode(device, &params);
	GFX_MarkLowLatencyParametersUpToDate();
}

//-----------------------------------------------------------------------------
// Purpose: runs a frame of the low latency sdk
// Input  : *device              - 
//-----------------------------------------------------------------------------
void GFX_RunLowLatencyFrame(IUnknown* device)
{
	const NvU64 currentFrameNumber = GFX_GetFrameNumber();

	if (s_ReflexLastFrameNumber == currentFrameNumber)
		return;

	if (GFX_ParameterUpdateWasSuccessful())
		NvAPI_D3D_Sleep(device);

	s_ReflexLastFrameNumber = currentFrameNumber;
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
	// TODO[ AMOS ]: should we keep calling this, even when the call to
	// 'NvAPI_D3D_SetSleepMode(...)' has failed?
	if (GFX_ParameterUpdateWasSuccessful())
	{
		NV_LATENCY_MARKER_PARAMS params = {};
		params.version = NV_LATENCY_MARKER_PARAMS_VER1;
		params.frameID = s_ReflexFrameNumber;
		params.markerType = markerType;

		NvAPI_D3D_SetLatencyMarker(device, &params);
	}
}
