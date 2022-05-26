#pragma once

#include <cstdint>
#include "MathHelper.h"
#include "Vector3.h"

namespace Math
{
	// Represents a 3D rotation (X/Y/Z/W)
	class Quaternion
	{
	public:
		Quaternion();
		Quaternion(float X, float Y, float Z, float W);

		float X;
		float Y;
		float Z;
		float W;

		// Array index operator
		float& operator[](int Index);

		// Logical operators
		Quaternion operator+(const Quaternion& Rhs) const;
		Quaternion operator-(const Quaternion& Rhs) const;
		Quaternion operator*(const Quaternion& Rhs) const;

		// Logical assignment operators
		Quaternion& operator+=(const Quaternion& Rhs);
		Quaternion& operator-=(const Quaternion& Rhs);
		Quaternion& operator*=(const Quaternion& Rhs);

		// Equality operator
		bool operator==(const Quaternion& Rhs) const;
		// Inequality operator
		bool operator!=(const Quaternion& Rhs) const;

		// Unary operator
		Quaternion operator-() const;
		// Conjugate operator
		Quaternion operator~() const;

		// Get the length of this instance
		float Length() const;
		// Get the length squared of this instance
		float LengthSq() const;
		// Normalize this instance
		void Normalize();
		// Get a normalized version of this instance
		Quaternion GetNormalized() const;

		// Convert this rotation to euler angles
		Vector3 ToEulerAngles() const;
		// Calculate the inverse of this instance
		Quaternion Inverse() const;

		// Quaternion slerp interpolation
		Quaternion Slerp(Quaternion& Rhs, float Time);

		// Convert this euler rotation to a quaternion rotation
		static Quaternion FromEulerAngles(float X, float Y, float Z);
		// Convert an axis rotation to a quaternion rotation
		static Quaternion FromAxisRotation(Vector3 Axis, float Angle);

		// Get an identity quaternion
		static Quaternion Identity();
	};

	static_assert(sizeof(Quaternion) == 0x10, "Invalid Math::Quaternion size, expected 0x10");
}