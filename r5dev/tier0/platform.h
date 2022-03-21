#ifndef PLATFORM_H
#define PLATFORM_H

#if defined( _WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
#pragma intrinsic(__rdtsc)
#endif

inline uint64_t Plat_Rdtsc()
{
#if defined( _X360 )
	return (uint64)__mftb32();
#elif defined( _WIN64 )
	return (uint64_t)__rdtsc();
#elif defined( _WIN32 )
#if defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
	return (uint64)__rdtsc();
#else
	__asm rdtsc;
	__asm ret;
#endif
#elif defined( __i386__ )
	uint64 val;
	__asm__ __volatile__("rdtsc" : "=A" (val));
	return val;
#elif defined( __x86_64__ )
	uint32 lo, hi;
	__asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
	return (((uint64)hi) << 32) | lo;
#else
#error
#endif
}

// Processor Information:
struct CPUInformation
{
	int	 m_Size; // Size of this structure, for forward compatability.

	uint8_t m_nLogicalProcessors;  // Number op logical processors.
	uint8_t m_nPhysicalProcessors; // Number of physical processors

	bool m_bRDTSC : 1, // Is RDTSC supported?
		m_bCMOV   : 1, // Is CMOV supported?
		m_bFCMOV  : 1, // Is FCMOV supported?
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

	char* m_szProcessorID;              // Processor vendor Identification.
	char* m_szProcessorBrand;           // Processor brand string, if available

	uint32_t m_nModel;
	uint32_t m_nFeatures[3];
	uint32_t m_nL1CacheSizeKb;
	uint32_t m_nL1CacheDesc;
	uint32_t m_nL2CacheSizeKb;
	uint32_t m_nL2CacheDesc;
	uint32_t m_nL3CacheSizeKb;
	uint32_t m_nL3CacheDesc;

	CPUInformation() : m_Size(0) {}
};

#endif /* PLATFORM_H */