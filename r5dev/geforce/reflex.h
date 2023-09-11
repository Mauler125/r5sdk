#ifndef GFSDK_REFLEX_H
#define GFSDK_REFLEX_H

void GFX_RunLowLatencySDK(IUnknown* device, const bool useLowLatencyMode,
	const bool useLowLatencyBoost, const float maxFramesPerSecond);

#endif // GFSDK_REFLEX_H
