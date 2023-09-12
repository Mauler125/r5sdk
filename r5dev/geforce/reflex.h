#ifndef GFSDK_REFLEX_H
#define GFSDK_REFLEX_H

int GFX_GetFrameNumber(void);

void GFX_RunLowLatencyFrame(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const bool useMarkersToOptimize,
	const float maxFramesPerSecond);

void GFX_SetLatencyMarker(IUnknown* device,
	const NV_LATENCY_MARKER_TYPE markerType);

#endif // GFSDK_REFLEX_H
