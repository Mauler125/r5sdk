#pragma once

#include <cstdint>
#include "MathHelper.h"
#include "Vector3.h"
#include "Quaternion.h"

namespace Math
{
	// Represents a 4x4 matrix
	class Matrix
	{
	public:
		Matrix();

		// Logical operators
		Matrix operator+(const Matrix& Rhs) const;
		Matrix operator-(const Matrix& Rhs) const;
		Matrix operator*(const Matrix& Rhs) const;

		// Scale logical operators
		Matrix operator/(float Rhs) const;

		// Equality operator
		bool operator==(const Matrix& Rhs) const;
		// Inequality operator
		bool operator!=(const Matrix& Rhs) const;

		// Get a value from a normal matrix
		float& Mat(int X, int Y);
		// Get a value from a normal matrix
		const float& Mat(int X, int Y) const;

		// Gets a pointer to the matrix 4x4 data
		float* GetMatrix();

		// Calculates the determinant of this instance
		float Determinant();
		// Calculates the inverse of this instance
		Matrix Inverse();

		// Gets the position of this instance
		Vector3 Position();
		// Gets the rotation of this instance
		Quaternion Rotation();
		// Gets the scale of this instance
		Vector3 Scale();

		// Sets the position of this instance
		void SetPosition(const Vector3& Value);
		// Sets the rotaton of this instance
		void SetRotation(const Quaternion& Rotation);
		// Sets the scale of this instance
		void SetScale(const Vector3& Scale);

		// TODO: SetScale

		// Create a rotation matrix from a quaternion rotation
		static Matrix CreateFromQuaternion(const Quaternion& Rotation);
		// Transform a vector3 by the given matrix
		static Vector3 TransformVector(const Vector3& Vector, const Matrix& Value);
		// Creates a matrix look at
		static Matrix CreateLookAt(const Vector3& From, const Vector3& To, const Vector3& Right);
		// Creates a matrix perspective FOV
		static Matrix CreatePerspectiveFov(float Fov, float Aspect, float Near, float Far);
		// Creates a matrix orthographic
		static Matrix CreateOrthographic(float Left, float Right, float Bottom, float Top, float Near, float Far);

	private:
		// Aligned matrix 4x4 buffer
		__declspec(align(16)) float _Data[16];
	};

	static_assert(sizeof(Matrix) == 0x40, "Invalid Math::Matrix size, expected 0x40");
}