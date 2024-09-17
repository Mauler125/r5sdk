#ifndef RDSDK_ANTILAG_H
#define RDSDK_ANTILAG_H

void Radeon_EnableLowLatencySDK(const bool enable);
bool Radeon_IsLowLatencySDKAvailable(void);

bool Radeon_InitLowLatencySDK();
void Radeon_ShutdownLowLatencySDK();

void Radeon_RunLowLatencyFrame(const unsigned int maxFPS);

#endif // RDSDK_ANTILAG_H
