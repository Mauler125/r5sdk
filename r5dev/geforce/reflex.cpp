//===========================================================================//
//
// Purpose: NVIDIA Reflex utilities
//
//===========================================================================//
#include "reflex.h"
#include "mathlib/mathlib.h"
#include "materialsystem/cmaterialsystem.h"

static bool s_LowLatencySDKEnabled = false;

// If false, the system will call 'NvAPI_D3D_SetSleepMode' to update the parameters.
bool s_ReflexModeInfoUpToDate = false;

// This is 'NVAPI_OK' If the call to 'NvAPI_D3D_SetSleepMode' was successful.
// If not, the Low Latency SDK will not run.
NvAPI_Status s_ReflexModeUpdateStatus = NvAPI_Status::NVAPI_OK;

//-----------------------------------------------------------------------------
// Purpose: enable/disable low latency SDK
// Input  : enable - 
//-----------------------------------------------------------------------------
void GFX_EnableLowLatencySDK(const bool enable)
{
	s_LowLatencySDKEnabled = enable;
}

//-----------------------------------------------------------------------------
// Purpose: whether we should run the low latency SDK
//-----------------------------------------------------------------------------
bool GFX_IsLowLatencySDKEnabled(void)
{
	if (!s_LowLatencySDKEnabled)
		return false;

	const MaterialAdapterInfo_t& adapterInfo = g_pMaterialAdapterMgr->GetAdapterInfo();
	// Only run on NVIDIA display drivers; AMD and Intel are not
	// supported by NVIDIA Reflex.
	return adapterInfo.m_VendorID == NVIDIA_VENDOR_ID;
}

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
// Purpose: updates the low latency parameters
// Input  : *device              - 
//          useLowLatencyMode    - 
//          useLowLatencyBoost   - 
//          useMarkersToOptimize - 
//          maxFramesPerSecond   - 
//-----------------------------------------------------------------------------
void GFX_UpdateLowLatencyParameters(IUnknown* const device, const bool useLowLatencyMode,
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
void GFX_RunLowLatencyFrame(IUnknown* const device)
{
	Assert(device);

	if (GFX_ParameterUpdateWasSuccessful())
		NvAPI_D3D_Sleep(device);
}

//-----------------------------------------------------------------------------
// Purpose: sets the latency marker
// Input  : *device        - 
//          frameNumber    - 
//          markerType     - 
//-----------------------------------------------------------------------------
void GFX_SetLatencyMarker(IUnknown* const device,
	const NV_LATENCY_MARKER_TYPE markerType, const NvU64 frameID)
{
	Assert(device);

	if (GFX_ParameterUpdateWasSuccessful() && GFX_IsLowLatencySDKEnabled())
	{
		NV_LATENCY_MARKER_PARAMS params = {};
		params.version = NV_LATENCY_MARKER_PARAMS_VER1;
		params.frameID = frameID;
		params.markerType = markerType;

		NvAPI_D3D_SetLatencyMarker(device, &params);
	}

	// PCLStats runs separately and is supported on non-NVIDIA hardware.
	PCLSTATS_MARKER(markerType, frameID);
}
