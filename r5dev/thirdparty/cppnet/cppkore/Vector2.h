#pragma once

#include <cstdint>
#include "MathHelper.h"

namespace Math
{
	// Represents a 2D vector with two points (X/Y)
	class Vector2
	{
	public:
		Vector2();
		Vector2(float X, float Y);

		union
		{
			float X;
			float U;
		};

		union
		{
			float Y;
			float V;
		};

		// Array index operator
		float& operator[](int Index);

		// Logical operators
		Vector2 operator+(const Vector2& Rhs) const;
		Vector2 operator-(const Vector2& Rhs) const;
		Vector2 operator*(const Vector2& Rhs) const;
		Vector2 operator/(const Vector2& Rhs) const;

		// Logical assignment operators
		Vector2& operator+=(const Vector2& Rhs);
		Vector2& operator-=(const Vector2& Rhs);
		Vector2& operator*=(const Vector2& Rhs);
		Vector2& operator/=(const Vector2& Rhs);

		// Scale logical operators
		Vector2 operator+(const float Rhs) const;
		Vector2 operator-(const float Rhs) const;
		Vector2 operator*(const float Rhs) const;
		Vector2 operator/(const float Rhs) const;

		// Scale assignment operators
		Vector2& operator+=(const float Rhs);
		Vector2& operator-=(const float Rhs);
		Vector2& operator*=(const float Rhs);
		Vector2& operator/=(const float Rhs);

		// Equality operator
		bool operator==(const Vector2& Rhs) const;
		// Inequality operator
		bool operator!=(const Vector2& Rhs) const;

		// Unary operator
		Vector2 operator-() const;

		// Get the length of this instance
		float Length() const;
		// Get the length squared of this instance
		float LengthSq() const;
		// Normalize this instance
		void Normalize();
		// Get a normalized version of this instance
		Vector2 GetNormalized() const;
		// Linear interpolation against another instance
		Vector2 Lerp(float Factor, const Vector2& Rhs) const;

		// Represents a 2D vector set to 0.0
		static Vector2 Zero();
		// Represents a 2D vector set to 1.0
		static Vector2 One();
	};
}