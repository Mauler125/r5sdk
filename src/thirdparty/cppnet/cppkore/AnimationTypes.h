#pragma once

#include <cstdint>

namespace Assets
{
	// This enumeration represents the possible animation types
	enum class AnimationCurveMode
	{
		// Animation translations are set to this exact value each frame.
		Absolute = 0,
		// Animation values are added on to the scene values.
		Additive = 1,
		// Animation values are relative to rest position in the model.
		Relative = 2
	};

	// This enumeration represents the possible transform spaces
	enum class AnimationTransformSpace
	{
		// All of the curve nodes are in local object space
		Local = 0,
		// All of the curve nodes are in world space
		World = 1,
	};

	// This enumeration represents the interpolation options
	enum class AnimationRotationInterpolation
	{
		Quaternion = 0,
		Euler = 1,
	};
}