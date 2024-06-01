//===========================================================================//
//
// Purpose: basic endian swap utils.
//
//===========================================================================//
#pragma once

#define LittleShort( val )			( val )
#define LittleWord( val )			( val )
#define LittleLong( val )			( val )
#define LittleDWord( val )			( val )
#define LittleQWord( val )			( val )

// If a swapped float passes through the fpu, the bytes may get changed.
// Prevent this by swapping floats as DWORDs.
#define SafeSwapFloat( pOut, pIn )	(*((uint*)pOut) = DWordSwap( *((uint*)pIn) ))

template <typename T>
inline T WordSwapC(T w)
{
	std::uint16_t swap;

	static_assert(sizeof(T) == sizeof(std::uint16_t));

	swap = ((*((std::uint16_t*)&w) & 0xff00) >> 8);
	swap |= ((*((std::uint16_t*)&w) & 0x00ff) << 8);

	return *((T*)&swap);
}

template <typename T>
inline T DWordSwapC(T dw)
{
	std::uint32_t swap;

	static_assert(sizeof(T) == sizeof(std::uint32_t));

	swap = *((std::uint32_t*)&dw) >> 24;
	swap |= ((*((std::uint32_t*)&dw) & 0x00FF0000) >> 8);
	swap |= ((*((std::uint32_t*)&dw) & 0x0000FF00) << 8);
	swap |= ((*((std::uint32_t*)&dw) & 0x000000FF) << 24);

	return *((T*)&swap);
}

template <typename T>
inline T QWordSwapC(T dw)
{
	static_assert(sizeof(dw) == sizeof(std::uint64_t));

	std::uint64_t swap;

	swap = *((std::uint64_t*)&dw) >> 56;
	swap |= ((*((std::uint64_t*)&dw) & 0x00FF000000000000ull) >> 40);
	swap |= ((*((std::uint64_t*)&dw) & 0x0000FF0000000000ull) >> 24);
	swap |= ((*((std::uint64_t*)&dw) & 0x000000FF00000000ull) >> 8);
	swap |= ((*((std::uint64_t*)&dw) & 0x00000000FF000000ull) << 8);
	swap |= ((*((std::uint64_t*)&dw) & 0x0000000000FF0000ull) << 24);
	swap |= ((*((std::uint64_t*)&dw) & 0x000000000000FF00ull) << 40);
	swap |= ((*((std::uint64_t*)&dw) & 0x00000000000000FFull) << 56);

	return *((T*)&swap);
}

#if PLAT_BIG_ENDIAN
#if defined( _PS3 )
inline uint32 LoadLittleDWord(uint32* base, unsigned int dwordIndex)
{
	return __lwbrx(base + dwordIndex);
}

inline void StoreLittleDWord(uint32* base, unsigned int dwordIndex, uint32 dword)
{
	__stwbrx(base + dwordIndex, dword);
}
inline uint64 LoadLittleInt64(uint64* base, unsigned int nWordIndex)
{
	return __ldbrx(base + nWordIndex);
}

inline void StoreLittleInt64(uint64* base, unsigned int nWordIndex, uint64 nWord)
{
	__stdbrx(base + nWordIndex, nWord);
}
#else
inline uint32 LoadLittleDWord(uint32* base, unsigned int dwordIndex)
{
	return __loadwordbytereverse(dwordIndex << 2, base);
}

inline void StoreLittleDWord(uint32* base, unsigned int dwordIndex, uint32 dword)
{
	__storewordbytereverse(dword, dwordIndex << 2, base);
}
inline uint64 LoadLittleInt64(uint64* base, unsigned int nWordIndex)
{
	return __loaddoublewordbytereverse(nWordIndex << 2, base);
}

inline void StoreLittleInt64(uint64* base, unsigned int nWordIndex, uint64 nWord)
{
	__storedoublewordbytereverse(nWord, nWordIndex << 2, base);
}
#endif

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		( *pOut = *pIn )
#define LittleFloat( pOut, pIn )	SafeSwapFloat( pOut, pIn )
#define SwapFloat( pOut, pIn )		LittleFloat( pOut, pIn )

#else
inline uint32 LoadLittleDWord(uint32* base, unsigned int dwordIndex)
{
	return LittleDWord(base[dwordIndex]);
}

inline void StoreLittleDWord(uint32* base, unsigned int dwordIndex, uint32 dword)
{
	base[dwordIndex] = LittleDWord(dword);
}

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		SafeSwapFloat( pOut, pIn )
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )
#define SwapFloat( pOut, pIn )		BigFloat( pOut, pIn )
#endif
