#pragma once

#include <cstdint>

namespace Assets
{
	// A container class that holds 3D face data.
	class Face
	{
	public:
		// Initialize a blank 3D face.
		Face();
		// Initialize a 3D face with the given vertices.
		Face(uint32_t Index1, uint32_t Index2, uint32_t Index3);
		// Destroys all 3D face data.
		~Face() = default;

		// Array index operator
		uint32_t& operator[](uint32_t Index);
		// Const array index operator
		const uint32_t& operator[](uint32_t Index) const;

	private:
		// Internal index buffer
		uint32_t Indices[3];
	};
}