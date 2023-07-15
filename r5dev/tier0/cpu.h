//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef CPU_H
#define CPU_H

//-----------------------------------------------------------------------------
// Processor Information:
//-----------------------------------------------------------------------------
struct CPUInformation
{
	int	 m_Size; // Size of this structure, for forward compatibility.

	uint8_t m_nLogicalProcessors;  // Number op logical processors.
	uint8_t m_nPhysicalProcessors; // Number of physical processors.

	bool m_bRDTSC : 1, // Is RDTSC supported?
		m_bCMOV   : 1, // Is CMOV supported?
		m_bFCMOV  : 1, // Is FCMOV supported?
		m_bPOPCNT : 1, // Is POPCNT supported?
		m_bSSE    : 1, // Is SSE supported?
		m_bSSE2   : 1, // Is SSE2 Supported?
		m_b3DNow  : 1, // Is 3DNow! Supported?
		m_bMMX    : 1, // Is MMX supported?
		m_bHT     : 1; // Is HyperThreading supported?


	bool m_bSSE3 : 1,
		m_bSSSE3 : 1,
		m_bSSE4a : 1,
		m_bSSE41 : 1,
		m_bSSE42 : 1,
		m_bAVX : 1;  // Is AVX supported?

	int64_t m_Speed;                    // In cycles per second.

	// Any member below doesn't exist in the game engine!
	char* m_szProcessorID;              // Processor vendor Identification.
	char* m_szProcessorBrand;           // Processor brand string, if available.

	uint32_t m_nModel;
	uint32_t m_nFeatures[3];
	uint32_t m_nL1CacheSizeKb;
	uint32_t m_nL1CacheDesc;
	uint32_t m_nL2CacheSizeKb;
	uint32_t m_nL2CacheDesc;
	uint32_t m_nL3CacheSizeKb;
	uint32_t m_nL3CacheDesc;

	CPUInformation() : m_Size(0)
	{
		m_Size = 0;

		m_nLogicalProcessors = 0;
		m_nPhysicalProcessors = 0;

		m_bRDTSC = false;
		m_bCMOV  = false;
		m_bCMOV  = false;
		m_bFCMOV = false;
		m_bSSE   = false;
		m_bSSE2  = false;
		m_b3DNow = false;
		m_bMMX   = false;
		m_bHT    = false;

		m_Speed = NULL;

		m_szProcessorID    = nullptr;
		m_szProcessorBrand = nullptr;

		m_nModel = 0;
		m_nFeatures[0] = 0;
		m_nFeatures[1] = 0;
		m_nFeatures[2] = 0;

		m_nL1CacheSizeKb = 0;
		m_nL1CacheDesc   = 0;
		m_nL2CacheSizeKb = 0;
		m_nL2CacheDesc   = 0;
		m_nL3CacheSizeKb = 0;
		m_nL3CacheDesc   = 0;
	}
};

bool CheckSSE3Technology(void);
bool CheckSSSE3Technology(void);
bool CheckSSE41Technology(void);
bool CheckSSE42Technology(void);
bool CheckSSE4aTechnology(void);

const char* GetProcessorVendorId(void);
const char* GetProcessorBrand(bool bRemovePadding);

const CPUInformation& GetCPUInformation(void);

#endif // CPU_H
