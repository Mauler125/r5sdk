//===========================================================================//
//
// Purpose: basic endian swap utils.
//
//===========================================================================//
#pragma once

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
