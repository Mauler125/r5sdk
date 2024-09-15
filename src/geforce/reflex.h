#ifndef GFSDK_REFLEX_H
#define GFSDK_REFLEX_H

extern bool g_PCLStatsAvailable;

void GeForce_EnableLowLatencySDK(const bool enable);
bool GeForce_IsLowLatencySDKEnabled(void);

void GeForce_MarkLowLatencyParametersOutOfDate(void);
bool GeForce_HasPendingLowLatencyParameterUpdates(void);

void GeForce_UpdateLowLatencyParameters(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const bool useMarkersToOptimize,
	const float maxFramesPerSecond);

void GeForce_RunLowLatencyFrame(IUnknown* device);

void GeForce_SetLatencyMarker(IUnknown* device,
	const NV_LATENCY_MARKER_TYPE markerType, const NvU64 frameID);

#endif // GFSDK_REFLEX_H
