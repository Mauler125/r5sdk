#include "stdafx.h"
#include "Half.h"

namespace Math
{
	Half::Half()
		: Half(0.0f)
	{
	}

	Half::Half(float Value)
	{
		FloatBits v, s;
		v.f = Value;

		uint32_t sign = v.si & signN;
		v.si ^= sign;

		sign >>= shiftSign;
		s.si = mulN;

		s.si = (int32_t)(s.f * v.f);
		v.si ^= (s.si ^ v.si) & -(minN > v.si);
		v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
		v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));

		v.ui >>= shift;
		v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
		v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);

		this->Value = (v.ui | sign);
	}

	Half::Half(uint16_t Value)
		: Value(Value)
	{
	}

	float Half::ToFloat()
	{
		FloatBits v, s;
		v.ui = this->Value;

		int32_t sign = v.si & signC;
		v.si ^= sign;
		sign <<= shiftSign;
		v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
		v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);

		s.si = mulC;
		s.f *= v.si;
		int32_t mask = -(norC > v.si);
		v.si <<= shift;
		v.si ^= (s.si ^ v.si) & mask;
		v.si |= sign;

		return v.f;
	}

	uint16_t Half::ToHalf()
	{
		return this->Value;
	}

	uint16_t Half::ToHalf(float Value)
	{
		return Half(Value).ToHalf();
	}

	float Half::ToFloat(uint16_t Value)
	{
		return Half(Value).ToFloat();
	}
}
