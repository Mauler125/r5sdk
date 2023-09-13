#ifndef GFSDK_REFLEX_H
#define GFSDK_REFLEX_H

void GFX_MarkLowLatencyParametersOutOfDate(void);
bool GFX_HasPendingLowLatencyParameterUpdates(void);

NvU64 GFX_GetFrameNumber(void);
void GFX_IncrementFrameNumber(void);

void GFX_UpdateLowLatencyParameters(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const bool useMarkersToOptimize,
	const float maxFramesPerSecond);

void GFX_RunLowLatencyFrame(IUnknown* device);

void GFX_SetLatencyMarker(IUnknown* device,
	const NV_LATENCY_MARKER_TYPE markerType);

#endif // GFSDK_REFLEX_H
