#include "stdafx.h"
#include "Face.h"

namespace Assets
{
	Face::Face()
		: Face(0, 0, 0)
	{
	}

	Face::Face(uint32_t Index1, uint32_t Index2, uint32_t Index3)
		: Indices{ Index1, Index2, Index3 }
	{
	}

	uint32_t& Face::operator[](uint32_t Index)
	{
		return this->Indices[Index];
	}

	const uint32_t& Face::operator[](uint32_t Index) const
	{
		return this->Indices[Index];
	}
}
