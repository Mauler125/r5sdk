//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "tier0/cpu.h"
#include "tier0/cputopology.h"
#include "tier0/fasttimer.h"

/*******************************************************************************/
static CPUInformation s_cpuInformation;
static char s_CpuVendorID[13]  = "unknown";
bool s_bCpuBrandInitialized    = false;
bool s_bCpuVendorIdInitialized = false;
static CpuBrand_t s_CpuBrand;

/*******************************************************************************/
struct IntelCacheDesc_t
{
	uint8_t nDesc;
	uint16_t nCacheSize;
};

/*******************************************************************************/
inline static IntelCacheDesc_t s_IntelL1DataCacheDesc[] = {
	{ 0xA, 8 },
	{ 0xC, 16 },
	{ 0xD, 16 },
	{ 0x2C, 32 },
	{ 0x30, 32 },
	{ 0x60, 16 },
	{ 0x66, 8 },
	{ 0x67, 16 },
	{ 0x68, 32 }
};


inline static IntelCacheDesc_t s_IntelL2DataCacheDesc[] =
{
	{ 0x21, 256 },
	{ 0x39, 128 },
	{ 0x3a, 192 },
	{ 0x3b, 128 },
	{ 0x3c, 256 },
	{ 0x3D, 384 },
	{ 0x3E, 512 },
	{ 0x41, 128 },
	{ 0x42, 256 },
	{ 0x43, 512 },
	{ 0x44, 1024 },
	{ 0x45, 2048 },
	{ 0x48, 3 * 1024 },
	{ 0x4e, 6 * 1024 },
	{ 0x78, 1024 },
	{ 0x79, 128 },
	{ 0x7a, 256 },
	{ 0x7b, 512 },
	{ 0x7c, 1024 },
	{ 0x7d, 2048 },
	{ 0x7f, 512 },
	{ 0x82, 256 },
	{ 0x83, 512 },
	{ 0x84, 1024 },
	{ 0x85, 2048 },
	{ 0x86, 512 },
	{ 0x87, 1024 }
};


inline static IntelCacheDesc_t s_IntelL3DataCacheDesc[] = {
	{ 0x22, 512 },
	{ 0x23, 1024 },
	{ 0x25, 2 * 1024 },
	{ 0x29, 4 * 1024 },
	{ 0x46, 4 * 1024 },
	{ 0x47, 8 * 1024 },
	{ 0x49, 4 * 1024 }, // Only valid when: family == 0x0F && model == 0x06.
	{ 0x4a, 6 * 1024 },
	{ 0x4b, 8 * 1024 },
	{ 0x4c, 12 * 1024 },
	{ 0x4d, 16 * 1014 },
	{ 0xD0, 512 },
	{ 0xD1, 1024 },
	{ 0xD2, 2048 },
	{ 0xD6, 1024 },
	{ 0xD7, 2048 },
	{ 0xD8, 4096 },
	{ 0xDC, 1536 },
	{ 0xDD, 3 * 1024 },
	{ 0xDE, 6 * 1024 },
	{ 0xE2, 2048 },
	{ 0xE3, 4096 },
	{ 0xE4, 8 * 1024 },
	{ 0xEA, 12 * 1024 },
	{ 0xEB, 18 * 1024 },
	{ 0xEC, 24 * 1024 }
};

/*******************************************************************************/
static bool cpuid(unsigned long function, CpuIdResult_t& out)
{
	int pCPUInfo[4];
	__cpuid(pCPUInfo, static_cast<int>(function));
	out.eax = pCPUInfo[0];
	out.ebx = pCPUInfo[1];
	out.ecx = pCPUInfo[2];
	out.edx = pCPUInfo[3];
	return true;
}


static bool cpuidex(unsigned long function, unsigned long subfunction, CpuIdResult_t& out)
{
	int pCPUInfo[4];
	__cpuidex(pCPUInfo, static_cast<int>(function), static_cast<int>(subfunction));
	out.eax = pCPUInfo[0];
	out.ebx = pCPUInfo[1];
	out.ecx = pCPUInfo[2];
	out.edx = pCPUInfo[3];
	return true;
}


static CpuIdResult_t cpuid(unsigned long function)
{
	CpuIdResult_t out;
	if (!cpuid(function, out))
	{
		out.Reset();
	}
	return out;
}

static CpuIdResult_t cpuidex(unsigned long function, unsigned long subfunction)
{
	CpuIdResult_t out;
	if (!cpuidex(function, subfunction, out))
	{
		out.Reset();
	}
	return out;
}

/*******************************************************************************/
bool CheckSSETechnology(void)
{
	return (cpuid(1).edx & 0x2000000L) != 0;
}

bool CheckSSE2Technology(void)
{
	return (cpuid(1).edx & 0x04000000) != 0;
}

bool CheckSSE3Technology(void)
{
	return (cpuid(1).ecx & 0x00000001) != 0;	// bit 1 of ECX.
}

bool CheckSSSE3Technology(void)
{
	// SSSE 3 is implemented by both Intel and AMD.
	// Detection is done the same way for both vendors.
	return (cpuid(1).ecx & (1 << 9)) != 0;	// bit 9 of ECX.
}

bool CheckSSE41Technology(void)
{
	// SSE 4.1 is implemented by both Intel and AMD.
	// Detection is done the same way for both vendors.

	return (cpuid(1).ecx & (1 << 19)) != 0;	// bit 19 of ECX.
}

bool CheckSSE42Technology(void)
{
	// SSE4.2 is an Intel-only feature.

	const char* pchVendor = GetProcessorVendorId();
	if (0 != _stricmp(pchVendor, "GenuineIntel"))
	{
		return false;
	}

	return (cpuid(1).ecx & (1 << 20)) != 0;	// bit 20 of ECX.
}


bool CheckSSE4aTechnology(void)
{
	// SSE 4a is an AMD-only feature.

	const char* pchVendor = GetProcessorVendorId();
	if (0 != _stricmp(pchVendor, "AuthenticAMD"))
	{
		return false;
	}

	return (cpuid(1).ecx & (1 << 6)) != 0;	// bit 6 of ECX.
}


bool Check3DNowTechnology(void)
{
	if (cpuid(0x80000000).eax > 0x80000000L)
	{
		return (cpuid(0x80000001).eax & (1 << 31)) != 0;
	}
	return false;
}

bool CheckCMOVTechnology(void)
{
	return (cpuid(1).edx & (1 << 15)) != 0;
}

bool CheckFCMOVTechnology(void)
{
	return (cpuid(1).edx & (1 << 16)) != 0;
}

bool CheckRDTSCTechnology(void)
{
	return (cpuid(1).edx & 0x10) != 0;
}

// Return the Processor's vendor identification string, or "Generic_x86" if it doesn't exist on this CPU.
const char* GetProcessorVendorId(void)
{
	if (s_bCpuVendorIdInitialized)
	{
		return s_CpuVendorID;
	}

	s_bCpuVendorIdInitialized = true;

	CpuIdResult_t cpuid0 = cpuid(0);

	memset(s_CpuVendorID, 0, sizeof(s_CpuVendorID));

	if (!cpuid0.eax)
	{
		strcpy(s_CpuVendorID, ("Generic_x86"));
	}
	else
	{
		memcpy(s_CpuVendorID + 0, &(cpuid0.ebx), sizeof(cpuid0.ebx));
		memcpy(s_CpuVendorID + 4, &(cpuid0.edx), sizeof(cpuid0.edx));
		memcpy(s_CpuVendorID + 8, &(cpuid0.ecx), sizeof(cpuid0.ecx));
	}

	return s_CpuVendorID;
}

const char* GetProcessorBrand(bool bRemovePadding = true)
{
	if (s_bCpuBrandInitialized)
	{
		return s_CpuBrand.name;
	}
	s_bCpuBrandInitialized = true;

	memset(&s_CpuBrand, '\0', sizeof(s_CpuBrand));

	const char* pchVendor = GetProcessorVendorId();
	if (0 == _stricmp(pchVendor, "AuthenticAMD") || 0 == _stricmp(pchVendor, "GenuineIntel"))
	{
		// AMD/Intel brand string.
		if (cpuid(0x80000000).eax >= 0x80000004)
		{
			s_CpuBrand.cpuid[0] = cpuid(0x80000002);
			s_CpuBrand.cpuid[1] = cpuid(0x80000003);
			s_CpuBrand.cpuid[2] = cpuid(0x80000004);
		}
	}

	if (bRemovePadding)
	{
		for (size_t i = sizeof(s_CpuBrand.name); i-- > 0; )
		{
			if (s_CpuBrand.name[i] != '\0')
			{
				if (s_CpuBrand.name[i] != ' ')
				{
					if (i < (sizeof(s_CpuBrand.name) - 1))
					{
						s_CpuBrand.name[i + 1] = '\0';
						break;
					}
				}
			}
		}
	}

	return s_CpuBrand.name;
}

/*******************************************************************************/
// Returns non-zero if Hyper-Threading Technology is supported on the processors and zero if not.
// If it's supported, it does not mean that it's been enabled. So we test another flag to see if it's enabled
// See Intel Processor Identification and the CPUID instruction Application Note 485.
// http://www.intel.com/Assets/PDF/appnote/241618.pdf
static bool HTSupported(void)
{
	enum {
		HT_BIT                = 0x10000000,// EDX[28] - Bit 28 set indicates Hyper-Threading Technology is supported in hardware.
		FAMILY_ID             = 0x0f00,    // EAX[11:8] - Bit 11 thru 8 contains family processor id.
		EXT_FAMILY_ID         = 0x0f00000, // EAX[23:20] - Bit 23 thru 20 contains extended family  processor id.
		FAMILY_ID_386         = 0x0300,
		FAMILY_ID_486         = 0x0400,    // EAX[8:12]  -  486, 487 and overdrive.
		FAMILY_ID_PENTIUM     = 0x0500,    // Pentium, Pentium OverDrive  60 - 200.
		FAMILY_ID_PENTIUM_PRO = 0x0600,    // P Pro, P II, P III, P M, Celeron M, Core Duo, Core Solo, Core2 Duo, Core2 Extreme, P D, Xeon model F,
		                                   // also 45-nm : Intel Atom, Core i7, Xeon MP ; see Intel Processor Identification and the CPUID instruction pg 20,21.
		FAMILY_ID_EXTENDED    = 0x0F00     // P IV, Xeon, Celeron D, P D, .
	};

	// This works on both newer AMD and Intel CPUs.
	CpuIdResult_t cpuid1 = cpuid(1);

	// Previously, we detected P4 specifically; now, we detect GenuineIntel with HT enabled in general.
	// if (((cpuid1.eax & FAMILY_ID) ==  FAMILY_ID_EXTENDED) || (cpuid1.eax & EXT_FAMILY_ID))

	//  Check to see if this is an Intel Processor with HT or CMT capability , and if HT/CMT is enabled.
	// ddk: This codef is actually correct: see example code at http://software.intel.com/en-us/articles/multi-core-detect/
	return (cpuid1.edx & HT_BIT) != 0 && // Genuine Intel Processor with Hyper-Threading Technology implemented.
		((cpuid1.ebx >> 16) & 0xFF) > 1; // Hyper-Threading OR Core Multi-Processing has been enabled.
}

// | Commented out as its currently unused, this is to avoid a compiler warning |
// | regarding unused function of static linkage.                               |
// Returns the number of logical processors per physical processors.
//static uint8_t LogicalProcessorsPerPackage(void)
//{
//	// EBX[23:16] indicate number of logical processors per package.
//	const unsigned NUM_LOGICAL_BITS = 0x00FF0000;
//
//	if (!HTSupported())
//	{
//		return 1;
//	}
//
//	return static_cast<uint8_t>(((cpuid(1).ebx & NUM_LOGICAL_BITS) >> 16));
//}

// Measure the processor clock speed by sampling the cycle count, waiting
// for some fraction of a second, then measuring the elapsed number of cycles.
static int64 CalculateClockSpeed(void)
{
	LARGE_INTEGER waitTime, startCount, curCount;
	CCycleCount start, end;

	// Take 1/32 of a second for the measurement.
	QueryPerformanceFrequency(&waitTime);
	int scale = 5;
	waitTime.QuadPart >>= scale;

	QueryPerformanceCounter(&startCount);
	start.Sample();
	do
	{
		QueryPerformanceCounter(&curCount);
	} while (curCount.QuadPart - startCount.QuadPart < waitTime.QuadPart);
	end.Sample();

	return (end.GetLongCycles() - start.GetLongCycles()) << scale;
}

static void FindIntelCacheDesc(uint8_t nDesc, const IntelCacheDesc_t* pDesc, int nDescCount, uint32_t& nCache, uint32_t& nCacheDesc)
{
	for (int i = 0; i < nDescCount; ++i)
	{
		const IntelCacheDesc_t& desc = pDesc[i];
		if (desc.nDesc == nDesc)
		{
			nCache = desc.nCacheSize;
			nCacheDesc = nDesc;
			break;
		}
	}
}

// See "Output of the CPUID instruction" from Intel, page 26.
static void InterpretIntelCacheDescriptors(uint32_t nPackedDesc, CPUInformation& pi)
{
	if (nPackedDesc & 0x80000000)
	{
		return; // This is a wrong descriptor.
	}
	for (int i = 0; i < 4; ++i)
	{
		uint8_t nDesc = nPackedDesc & 0xFF;
		FindIntelCacheDesc(nDesc, s_IntelL1DataCacheDesc, ARRAYSIZE(s_IntelL1DataCacheDesc), pi.m_nL1CacheSizeKb, pi.m_nL1CacheDesc);
		FindIntelCacheDesc(nDesc, s_IntelL2DataCacheDesc, ARRAYSIZE(s_IntelL2DataCacheDesc), pi.m_nL2CacheSizeKb, pi.m_nL2CacheDesc);
		FindIntelCacheDesc(nDesc, s_IntelL3DataCacheDesc, ARRAYSIZE(s_IntelL3DataCacheDesc), pi.m_nL3CacheSizeKb, pi.m_nL3CacheDesc);

		int nFamily = (pi.m_nModel >> 8) & 0xF;
		int nModel = (pi.m_nModel >> 4) & 0xF;
		if (nDesc == 0x49 && (nFamily != 0x0F || nModel != 0x06))
		{
			pi.m_nL3CacheSizeKb = 0;
			pi.m_nL3CacheDesc = 0;
		}

		nPackedDesc >>= 8;
	}
}


const CPUInformation& GetCPUInformation(void)
{
	CPUInformation& pi = s_cpuInformation;
	// Has the structure already been initialized and filled out?
	if (pi.m_Size == sizeof(pi))
	{
		return pi;
	}

	// Redundant, but just in case the user somehow messes with the size.
	memset(&pi, 0x0, sizeof(pi));

	// Fill out the structure, and return it: 
	pi.m_Size = sizeof(pi);

	// Grab the processor frequency:
	pi.m_Speed = CalculateClockSpeed();

	// Get the logical and physical processor counts:
	//pi.m_nLogicalProcessors = LogicalProcessorsPerPackage();

	bool bAuthenticAMD = (0 == _stricmp(GetProcessorVendorId(), "AuthenticAMD"));
	bool bGenuineIntel = !bAuthenticAMD && (0 == _stricmp(GetProcessorVendorId(), "GenuineIntel"));

	SYSTEM_INFO si;
	ZeroMemory(&si, sizeof(si));

	GetSystemInfo(&si);

	// Fixing: si.dwNumberOfProcessors is the number of logical processors according to experiments on i7, P4 and a DirectX sample (Aug'09).
	// This is contrary to MSDN documentation on GetSystemInfo().
	pi.m_nLogicalProcessors = uint8_t(si.dwNumberOfProcessors);

	CpuTopology topo;
	pi.m_nPhysicalProcessors = uint8_t(topo.NumberOfSystemCores());

	// Make sure I always report at least one, when running WinXP with the /ONECPU switch, 
	// it likes to report 0 processors for some reason.
	if (pi.m_nPhysicalProcessors == 0 && pi.m_nLogicalProcessors == 0)
	{
		assert(!"Missing CPU detection code for this processor.");
		pi.m_nPhysicalProcessors = 1;
		pi.m_nLogicalProcessors = 1;
	}

	CpuIdResult_t cpuid0 = cpuid(0);
	if (cpuid0.eax >= 1)
	{
		CpuIdResult_t cpuid1 = cpuid(1);
		uint32_t bFPU = cpuid1.edx & 1; // This should always be set on anything we support.
		// Determine Processor Features:
		pi.m_bRDTSC = (cpuid1.edx >> 4) & 1;
		pi.m_bCMOV  = (cpuid1.edx >> 15) & 1;
		pi.m_bFCMOV = (pi.m_bCMOV && bFPU) ? 1 : 0;
		pi.m_bMMX   = (cpuid1.edx >> 23) & 1;
		pi.m_bSSE   = (cpuid1.edx >> 25) & 1;
		pi.m_bSSE2  = (cpuid1.edx >> 26) & 1;
		pi.m_bSSE3  = cpuid1.ecx & 1;
		pi.m_bSSSE3 = (cpuid1.ecx >> 9) & 1;
		pi.m_bSSE4a = CheckSSE4aTechnology();
		pi.m_bSSE41 = (cpuid1.ecx >> 19) & 1;
		pi.m_bSSE42 = (cpuid1.ecx >> 20) & 1;
		pi.m_b3DNow = Check3DNowTechnology();
		pi.m_bPOPCNT= (cpuid1.ecx >> 23) & 1;
		pi.m_bAVX   = (cpuid1.ecx >> 28) & 1;
		pi.m_bHRVSR = (cpuid1.ecx >> 31) & 1;
		pi.m_szProcessorID = const_cast<char*>(GetProcessorVendorId());
		pi.m_szProcessorBrand = const_cast<char*>(GetProcessorBrand());
		pi.m_bHT = (pi.m_nPhysicalProcessors < pi.m_nLogicalProcessors); //HTSupported();

		pi.m_nModel = cpuid1.eax; // Full CPU model info.
		pi.m_nFeatures[0] = cpuid1.edx; // x87+ features.
		pi.m_nFeatures[1] = cpuid1.ecx; // sse3+ features.
		pi.m_nFeatures[2] = cpuid1.ebx; // Some additional features.

		if (bGenuineIntel)
		{
			if (cpuid0.eax >= 4)
			{
				// We have CPUID.4, use it to find all the cache parameters.
				const uint32_t nCachesToQuery = 4; // Level 0 is not used.
				uint32_t nCacheSizeKiB[nCachesToQuery]{};
				uint32_t nCacheDesc[nCachesToQuery]{};

				for (unsigned long nSub = 0; nSub < 1024; ++nSub)
				{
					CpuIdResult_t cpuid4 = cpuidex(4, nSub);
					uint32_t nCacheType = cpuid4.eax & 0x1F;
					if (nCacheType == 0)
					{
						// No more caches.
						break;
					}
					if (nCacheType & 1)
					{
						// This cache includes data cache: it's either data or unified. Instruction cache type is 2.
						uint32_t nCacheLevel = (cpuid4.eax >> 5) & 7;
						if (nCacheLevel < nCachesToQuery)
						{
							uint32_t nCacheWays        = 1 + ((cpuid4.ebx >> 22) & 0x3F);
							uint32_t nCachePartitions  = 1 + ((cpuid4.ebx >> 12) & 0x3F);
							uint32_t nCacheLineSize    = 1 + (cpuid4.ebx & 0xFF);
							uint32_t nCacheSets        = 1 + cpuid4.ecx;
							uint32_t nCacheSizeBytes   = nCacheWays * nCachePartitions * nCacheLineSize * nCacheSets;

							nCacheSizeKiB[nCacheLevel] = nCacheSizeBytes >> 10;
							nCacheDesc[nCacheLevel] = 1 + cpuid4.ebx;
						}
					}
				}

				pi.m_nL1CacheSizeKb = nCacheSizeKiB[1];
				pi.m_nL1CacheDesc = nCacheDesc[1];
				pi.m_nL2CacheSizeKb = nCacheSizeKiB[2];
				pi.m_nL2CacheDesc = nCacheDesc[2];
				pi.m_nL3CacheSizeKb = nCacheSizeKiB[3];
				pi.m_nL3CacheDesc = nCacheDesc[3];
			}
			else if (cpuid0.eax >= 2)
			{
				// Get the cache.
				CpuIdResult_t cpuid2 = cpuid(2);
				for (int i = (cpuid2.eax & 0xFF); i-- > 0; )
				{
					InterpretIntelCacheDescriptors(cpuid2.eax & ~0xFF, pi);
					InterpretIntelCacheDescriptors(cpuid2.ebx, pi);
					InterpretIntelCacheDescriptors(cpuid2.ecx, pi);
					InterpretIntelCacheDescriptors(cpuid2.edx, pi);
					cpuid2 = cpuid(2); // Read the next.
				}
			}
		}
	}

	CpuIdResult_t cpuid0ex = cpuid(0x80000000);
	if (bAuthenticAMD)
	{
		// TODO: add '0x8000001D' (newer AMD cpu's).
		// Fall back to below if value doesn't equal/exceed it.
		if (cpuid0ex.eax >= 0x80000005)
		{
			CpuIdResult_t cpuid5ex = cpuid(0x80000005);
			pi.m_nL1CacheSizeKb = cpuid5ex.ecx >> 24;
			pi.m_nL1CacheDesc = cpuid5ex.ecx & 0xFFFFFF;
		}
		if (cpuid0ex.eax >= 0x80000006)
		{
			CpuIdResult_t cpuid6ex = cpuid(0x80000006);
			pi.m_nL2CacheSizeKb = cpuid6ex.ecx >> 16;
			pi.m_nL2CacheDesc = cpuid6ex.ecx & 0xFFFF;
			pi.m_nL3CacheSizeKb = (cpuid6ex.edx >> 18) * 512;
			pi.m_nL3CacheDesc = cpuid6ex.edx & 0xFFFF;
		}
	}
	else if (bGenuineIntel)
	{
		if (cpuid0ex.eax >= 0x80000006)
		{
			// Make sure we got the L2 cache info right.
			CpuIdResult_t cpuid6ex = cpuid(0x80000006);

			pi.m_nL2CacheSizeKb = cpuid6ex.ecx >> 16;
			pi.m_nL2CacheDesc = cpuid6ex.ecx & 0xFFFF;
		}
	}
	return pi;
}

void CheckSystemCPUForSSE2()
{
	const CPUInformation& pi = GetCPUInformation();

	if (!(pi.m_bSSE && pi.m_bSSE2))
	{
		if (MessageBoxA(NULL, "SSE and SSE2 are required.", "Unsupported CPU", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), 0xFFFFFFFF);
		}
	}
}

void CheckSystemCPUForSSE3()
{
	const CPUInformation& pi = GetCPUInformation();

	if (!pi.m_bSSE3)
	{
		if (MessageBoxA(NULL, "SSE3 is required.", "Unsupported CPU", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), 0xFFFFFFFF);
		}
	}
}

void CheckSystemCPUForSupplementalSSE3()
{
	const CPUInformation& pi = GetCPUInformation();

	if (!pi.m_bSSSE3)
	{
		if (MessageBoxA(NULL, "SSSE3 (Supplemental SSE3 Instructions) is required.", "Unsupported CPU", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), 0xFFFFFFFF);
		}
	}
}

void CheckSystemCPUForPopCount()
{
	const CPUInformation& pi = GetCPUInformation();

	if (!pi.m_bPOPCNT)
	{
		if (MessageBoxA(NULL, "POPCNT is required.", "Unsupported CPU", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), 0xFFFFFFFF);
		}
	}
}

void CheckSystemCPU()
{
	CheckSystemCPUForSSE2();
	CheckSystemCPUForSSE3();
	CheckSystemCPUForSupplementalSSE3();
	CheckSystemCPUForPopCount();
}
