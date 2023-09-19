#pragma once

#include <cstdint>
#include <type_traits>

namespace Assets
{
	// This enumeration represents the possible bone flags.
	enum class BoneFlags : uint8_t
	{
		// Whether or not the bone has local space transforms
		HasLocalSpaceMatrices = 0x1,
		// Whether or not the bone has global space transforms
		HasGlobalSpaceMatrices = 0x2,
		// Whether or not the bone has a scale transform
		HasScale = 0x4
	};

	//
	// Allow bitwise operations on this enumeration
	//
	constexpr BoneFlags operator|(BoneFlags Lhs, BoneFlags Rhs)
	{
		return static_cast<BoneFlags>(static_cast<std::underlying_type<BoneFlags>::type>(Lhs) | static_cast<std::underlying_type<BoneFlags>::type>(Rhs));
	};
}