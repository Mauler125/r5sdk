#pragma once

#include <cstdint>

namespace Math
{
	// Represents a 16bit floating point value.
	class Half
	{
	public:
		Half();
		Half(float Value);
		Half(uint16_t Value);
		~Half() = default;

		// Converts the float 16bit value to a 32bit float.
		float ToFloat();
		// Converts the float 32bit value to a 16bit float.
		uint16_t ToHalf();
		
		// Converts the float 32bit value to a 16bit float.
		static uint16_t ToHalf(float Value);
		// Convert the float 16bit value to a 32bit float.
		static float ToFloat(uint16_t Value);

	private:
		// Internal value
		uint16_t Value;

		// Represents the different casts of a float
		union FloatBits
		{
			float f;
			int32_t si;
			uint32_t ui;
		};

		// Shift value
		static int constexpr shift = 13;
		// Sign used to shift
		static int constexpr shiftSign = 16;

		// Infinity of a float
		static int32_t constexpr infN = 0x7F800000;
		// Maximum value of a half float
		static int32_t constexpr maxN = 0x477FE000;
		// Minimum value of a half float
		static int32_t constexpr minN = 0x38800000;
		// Sign bit of a float
		static int32_t constexpr signN = 0x80000000;

		// Precalculated properties of a half float
		static int32_t constexpr infC = infN >> shift;
		static int32_t constexpr nanN = (infC + 1) << shift;
		static int32_t constexpr maxC = maxN >> shift;
		static int32_t constexpr minC = minN >> shift;
		static int32_t constexpr signC = signN >> shiftSign;

		// Precalculated (1 << 23) / minN
		static int32_t constexpr mulN = 0x52000000;
		// Precalculated minN / (1 << (23 - shift))
		static int32_t constexpr mulC = 0x33800000;

		// Max float subnormal down shifted
		static int32_t constexpr subC = 0x003FF;
		// Min float normal down shifted
		static int32_t constexpr norC = 0x00400;

		// Precalculated min and max decimals
		static int32_t constexpr maxD = infC - maxC - 1;
		static int32_t constexpr minD = minC - subC - 1;
	};

	// A 16bit floating point value.
	using half_t = Half;
}