//===========================================================================//
//
// Purpose: AMD Anti-Lag 2 utilities
//
//===========================================================================//
#include "antilag.h"
#include "mathlib/mathlib.h"
#include "materialsystem/cmaterialsystem.h"

static bool s_LowLatencySDKEnabled = false;

// This will be true if the call to 'AMD::AntiLag2DX11::Initialize' succeeds.
static bool s_LowLatencyAvailable = false;

static AMD::AntiLag2DX11::Context s_LowLatencyContext = {};

//-----------------------------------------------------------------------------
// Purpose: enable/disable low latency SDK
// Input  : enable - 
//-----------------------------------------------------------------------------
void Radeon_EnableLowLatencySDK(const bool enable)
{
	s_LowLatencySDKEnabled = enable;
}

//-----------------------------------------------------------------------------
// Purpose: whether we should run the low latency SDK
//-----------------------------------------------------------------------------
bool Radeon_IsLowLatencySDKAvailable(void)
{
	if (!s_LowLatencyAvailable)
		return false;

	const MaterialAdapterInfo_t& adapterInfo = g_pMaterialAdapterMgr->GetAdapterInfo();
	// Only run on AMD display drivers; NVIDIA and Intel are not
	// supported by AMD Anti-Lag 2.
	return adapterInfo.m_VendorID == AMD_VENDOR_ID;
}

//-----------------------------------------------------------------------------
// Purpose: initialize the low latency SDK
//-----------------------------------------------------------------------------
bool Radeon_InitLowLatencySDK(void)
{
	if (AMD::AntiLag2DX11::Initialize(&s_LowLatencyContext) == S_OK)
		s_LowLatencyAvailable = true;

	return s_LowLatencyAvailable;
}

//-----------------------------------------------------------------------------
// Purpose: shutdown the low latency SDK
//-----------------------------------------------------------------------------
void Radeon_ShutdownLowLatencySDK()
{
	if (AMD::AntiLag2DX11::DeInitialize(&s_LowLatencyContext) == 0)
		s_LowLatencyAvailable = false;
}

//-----------------------------------------------------------------------------
// Purpose: runs a frame of the low latency sdk
// Input  : maxFPS              - 
//-----------------------------------------------------------------------------
void Radeon_RunLowLatencyFrame(const unsigned int maxFPS)
{
	Assert(Radeon_IsLowLatencySDKAvailable());
	AMD::AntiLag2DX11::Update(&s_LowLatencyContext, s_LowLatencySDKEnabled, maxFPS);
}
