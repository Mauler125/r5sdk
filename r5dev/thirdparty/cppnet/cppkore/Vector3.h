#pragma once

#include <cstdint>
#include "MathHelper.h"

namespace Math
{
	// Represents a 3D vector with three points (X/Y/Z)
	class Vector3
	{
	public:
		Vector3();
		Vector3(float X, float Y, float Z);

		float X;
		float Y;
		float Z;

		// Array index operator
		float& operator[](int Index);

		// Logical operators
		Vector3 operator+(const Vector3& Rhs) const;
		Vector3 operator-(const Vector3& Rhs) const;
		Vector3 operator*(const Vector3& Rhs) const;
		Vector3 operator/(const Vector3& Rhs) const;

		// Logical assignment operators
		Vector3& operator+=(const Vector3& Rhs);
		Vector3& operator-=(const Vector3& Rhs);
		Vector3& operator*=(const Vector3& Rhs);
		Vector3& operator/=(const Vector3& Rhs);

		// Scale logical operators
		Vector3 operator+(const float Rhs) const;
		Vector3 operator-(const float Rhs) const;
		Vector3 operator*(const float Rhs) const;
		Vector3 operator/(const float Rhs) const;

		// Scale assignment operators
		Vector3& operator+=(const float Rhs);
		Vector3& operator-=(const float Rhs);
		Vector3& operator*=(const float Rhs);
		Vector3& operator/=(const float Rhs);

		// Equality operator
		bool operator==(const Vector3& Rhs) const;
		// Inequality operator
		bool operator!=(const Vector3& Rhs) const;

		// Unary operator
		Vector3 operator-() const;

		// Get the length of this instance
		float Length() const;
		// Get the length squared of this instance
		float LengthSq() const;
		// Normalize this instance
		void Normalize();
		// Get a normalized version of this instance
		Vector3 GetNormalized() const;
		// Compute the cross product of this vector and another
		Vector3 Cross(const Vector3& Rhs) const;
		// Compute the dot product of this vector and another
		float Dot(const Vector3& Rhs);
		// Linear interpolation against another instance
		Vector3 Lerp(float Factor, const Vector3& Rhs) const;

		// Represents a 3D vector set to 0.0
		static Vector3 Zero();
		// Represents a 3D vector set to 1.0
		static Vector3 One();
	};

	static_assert(sizeof(Vector3) == 0xC, "Invalid Math::Vector3 size, expected 0xC");
}

using Vector3 = Math::Vector3;