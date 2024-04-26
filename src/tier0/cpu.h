//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef CPU_H
#define CPU_H

//-----------------------------------------------------------------------------
// CPUID Result:
//-----------------------------------------------------------------------------
struct CpuIdResult_t
{
	unsigned long eax;
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;

	void Reset(void)
	{
		eax = ebx = ecx = edx = 0;
	}
};

//-----------------------------------------------------------------------------
// CPU Brand:
//-----------------------------------------------------------------------------
union CpuBrand_t
{
	CpuIdResult_t cpuid[3];
	char name[49];
};

//-----------------------------------------------------------------------------
// Processor Information:
//-----------------------------------------------------------------------------
struct CPUInformation
{
	int	 m_Size; // Size of this structure, for forward compatibility.

	bool m_bRDTSC : 1, // Is RDTSC supported?
		m_bCMOV   : 1, // Is CMOV supported?
		m_bFCMOV  : 1, // Is FCMOV supported?
		m_bSSE    : 1, // Is SSE supported?
		m_bSSE2   : 1, // Is SSE2 Supported?
		m_b3DNow  : 1, // Is 3DNow! Supported?
		m_bMMX    : 1, // Is MMX supported?
		m_bHT     : 1; // Is HyperThreading supported?

	uint8 m_nLogicalProcessors;  // Number op logical processors.
	uint8 m_nPhysicalProcessors; // Number of physical processors.

	int64	m_Speed;             // In cycles per second.
	char* m_szProcessorID;       // Processor vendor Identification.

	// !!! any member below here is SDK specific and does NOT exist in the game engine !!!
	char* m_szProcessorBrand;    // Processor brand string, if available.

	bool m_bSSE3 : 1,
		m_bSSSE3 : 1,
		m_bSSE4a : 1,
		m_bSSE41 : 1,
		m_bSSE42 : 1,
		m_bPOPCNT: 1, // Pop count
		m_bAVX   : 1, // Advanced Vector Extensions
		m_bHRVSR : 1; // Hypervisor

	uint32 m_nModel;
	uint32 m_nFeatures[3];
	uint32 m_nL1CacheSizeKb;
	uint32 m_nL1CacheDesc;
	uint32 m_nL2CacheSizeKb;
	uint32 m_nL2CacheDesc;
	uint32 m_nL3CacheSizeKb;
	uint32 m_nL3CacheDesc;

	CPUInformation() : m_Size(0)
	{
		m_Size = 0;

		m_bRDTSC = false;
		m_bCMOV  = false;
		m_bFCMOV = false;
		m_bSSE   = false;
		m_bSSE2  = false;
		m_b3DNow = false;
		m_bMMX   = false;
		m_bHT    = false;

		m_nLogicalProcessors = 0;
		m_nPhysicalProcessors = 0;

		m_Speed = NULL;

		m_szProcessorID    = nullptr;
		m_szProcessorBrand = nullptr;

		m_bSSE3   = false;
		m_bSSSE3  = false;
		m_bSSE4a  = false;
		m_bSSE41  = false;
		m_bSSE42  = false;
		m_bPOPCNT = false;
		m_bAVX    = false;
		m_bHRVSR  = false;

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

void CheckSystemCPUForSSE2();
void CheckSystemCPUForSSE3();
void CheckSystemCPUForSupplementalSSE3();
void CheckSystemCPUForPopCount();

void CheckSystemCPU();

#endif // CPU_H
