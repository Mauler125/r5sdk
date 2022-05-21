#include "stdafx.h"
#include "Matrix.h"

namespace Math
{
	Matrix::Matrix()
	{
		for (int i = 0; i < 16; i++)
			_Data[i] = (i % 5) ? 0.0f : 1.0f;
	}

	Matrix Matrix::operator+(const Matrix& Rhs) const
	{
		Matrix Result;

		for (int i = 0; i < 16; i++)
			Result._Data[i] = _Data[i] + Rhs._Data[i];

		return Result;
	}

	Matrix Matrix::operator-(const Matrix& Rhs) const
	{
		Matrix Result;

		for (int i = 0; i < 16; i++)
			Result._Data[i] = _Data[i] - Rhs._Data[i];

		return Result;
	}

	Matrix Matrix::operator*(const Matrix& Rhs) const
	{
		Matrix Result;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				float Buffer = 0;

				for (int k = 0; k < 4; k++)
				{
					Buffer += Rhs.Mat(i, k) * Mat(k, j);
				}

				Result.Mat(i, j) = Buffer;
			}
		}

		return Result;
	}

	Matrix Matrix::operator/(float Rhs) const
	{
		Matrix Result;

		for (int i = 0; i < 16; i++)
			Result._Data[i] = _Data[i] / Rhs;

		return Result;
	}

	bool Matrix::operator==(const Matrix & Rhs) const
	{
		for (int i = 0; i < 16; i++)
		{
			if (std::abs(_Data[i] - Rhs._Data[i]) >= MathHelper::Epsilon)
				return false;
		}

		return true;
	}

	bool Matrix::operator!=(const Matrix& Rhs) const
	{
		return !(*this == Rhs);
	}

	float& Matrix::Mat(int X, int Y)
	{
		return _Data[X * 4 + Y];
	}

	const float& Matrix::Mat(int X, int Y) const
	{
		return _Data[X * 4 + Y];
	}

	float* Matrix::GetMatrix()
	{
		return &this->_Data[0];
	}

	float Matrix::Determinant()
	{
		return Mat(3, 0) * Mat(2, 1) * Mat(1, 2) * Mat(0, 3) - Mat(2, 0) * Mat(3, 1) * Mat(1, 2) * Mat(0, 3)
			- Mat(3, 0) * Mat(1, 1) * Mat(2, 2) * Mat(0, 3) + Mat(1, 0) * Mat(3, 1) * Mat(2, 2) * Mat(0, 3)

			+ Mat(2, 0) * Mat(1, 1) * Mat(3, 2) * Mat(0, 3) - Mat(1, 0) * Mat(2, 1) * Mat(3, 2) * Mat(0, 3)
			- Mat(3, 0) * Mat(2, 1) * Mat(0, 2) * Mat(1, 3) + Mat(2, 0) * Mat(3, 1) * Mat(0, 2) * Mat(1, 3)

			+ Mat(3, 0) * Mat(0, 1) * Mat(2, 2) * Mat(1, 3) - Mat(0, 0) * Mat(3, 1) * Mat(2, 2) * Mat(1, 3)
			- Mat(2, 0) * Mat(0, 1) * Mat(3, 2) * Mat(1, 3) + Mat(0, 0) * Mat(2, 1) * Mat(3, 2) * Mat(1, 3)

			+ Mat(3, 0) * Mat(1, 1) * Mat(0, 2) * Mat(2, 3) - Mat(1, 0) * Mat(3, 1) * Mat(0, 2) * Mat(2, 3)
			- Mat(3, 0) * Mat(0, 1) * Mat(1, 2) * Mat(2, 3) + Mat(0, 0) * Mat(3, 1) * Mat(1, 2) * Mat(2, 3)

			+ Mat(1, 0) * Mat(0, 1) * Mat(3, 2) * Mat(2, 3) - Mat(0, 0) * Mat(1, 1) * Mat(3, 2) * Mat(2, 3)
			- Mat(2, 0) * Mat(1, 1) * Mat(0, 2) * Mat(3, 3) + Mat(1, 0) * Mat(2, 1) * Mat(0, 2) * Mat(3, 3)

			+ Mat(2, 0) * Mat(0, 1) * Mat(1, 2) * Mat(3, 3) - Mat(0, 0) * Mat(2, 1) * Mat(1, 2) * Mat(3, 3)
			- Mat(1, 0) * Mat(0, 1) * Mat(2, 2) * Mat(3, 3) + Mat(0, 0) * Mat(1, 1) * Mat(2, 2) * Mat(3, 3);
	}

	Matrix Matrix::Inverse()
	{
		Matrix Result;

		Result.Mat(0, 0) = + Mat(2, 1) * Mat(3, 2) * Mat(1, 3) - Mat(3, 1) * Mat(2, 2) * Mat(1, 3) + Mat(3, 1) * Mat(1, 2) * Mat(2, 3)
			- Mat(1, 1) * Mat(3, 2) * Mat(2, 3) - Mat(2, 1) * Mat(1, 2) * Mat(3, 3) + Mat(1, 1) * Mat(2, 2) * Mat(3, 3);

		Result.Mat(1, 0) = + Mat(3, 0) * Mat(2, 2) * Mat(1, 3) - Mat(2, 0) * Mat(3, 2) * Mat(1, 3) - Mat(3, 0) * Mat(1, 2) * Mat(2, 3)
			+ Mat(1, 0) * Mat(3, 2) * Mat(2, 3) + Mat(2, 0) * Mat(1, 2) * Mat(3, 3) - Mat(1, 0) * Mat(2, 2) * Mat(3, 3);

		Result.Mat(2, 0) = + Mat(2, 0) * Mat(3, 1) * Mat(1, 3) - Mat(3, 0) * Mat(2, 1) * Mat(1, 3) + Mat(3, 0) * Mat(1, 1) * Mat(2, 3)
			- Mat(1, 0) * Mat(3, 1) * Mat(2, 3) - Mat(2, 0) * Mat(1, 1) * Mat(3, 3) + Mat(1, 0) * Mat(2, 1) * Mat(3, 3);

		Result.Mat(3, 0) = + Mat(3, 0) * Mat(2, 1) * Mat(1, 2) - Mat(2, 0) * Mat(3, 1) * Mat(1, 2) - Mat(3, 0) * Mat(1, 1) * Mat(2, 2)
			+ Mat(1, 0) * Mat(3, 1) * Mat(2, 2) + Mat(2, 0) * Mat(1, 1) * Mat(3, 2) - Mat(1, 0) * Mat(2, 1) * Mat(3, 2);

		Result.Mat(0, 1) = + Mat(3, 1) * Mat(2, 2) * Mat(0, 3) - Mat(2, 1) * Mat(3, 2) * Mat(0, 3) - Mat(3, 1) * Mat(0, 2) * Mat(2, 3)
			+ Mat(0, 1) * Mat(3, 2) * Mat(2, 3) + Mat(2, 1) * Mat(0, 2) * Mat(3, 3) - Mat(0, 1) * Mat(2, 2) * Mat(3, 3);

		Result.Mat(1, 1) = + Mat(2, 0) * Mat(3, 2) * Mat(0, 3) - Mat(3, 0) * Mat(2, 2) * Mat(0, 3) + Mat(3, 0) * Mat(0, 2) * Mat(2, 3)
			- Mat(0, 0) * Mat(3, 2) * Mat(2, 3) - Mat(2, 0) * Mat(0, 2) * Mat(3, 3) + Mat(0, 0) * Mat(2, 2) * Mat(3, 3);

		Result.Mat(2, 1) = + Mat(3, 0) * Mat(2, 1) * Mat(0, 3) - Mat(2, 0) * Mat(3, 1) * Mat(0, 3) - Mat(3, 0) * Mat(0, 1) * Mat(2, 3)
			+ Mat(0, 0) * Mat(3, 1) * Mat(2, 3) + Mat(2, 0) * Mat(0, 1) * Mat(3, 3) - Mat(0, 0) * Mat(2, 1) * Mat(3, 3);

		Result.Mat(3, 1) = + Mat(2, 0) * Mat(3, 1) * Mat(0, 2) - Mat(3, 0) * Mat(2, 1) * Mat(0, 2) + Mat(3, 0) * Mat(0, 1) * Mat(2, 2)
			- Mat(0, 0) * Mat(3, 1) * Mat(2, 2) - Mat(2, 0) * Mat(0, 1) * Mat(3, 2) + Mat(0, 0) * Mat(2, 1) * Mat(3, 2);

		Result.Mat(0, 2) = + Mat(1, 1) * Mat(3, 2) * Mat(0, 3) - Mat(3, 1) * Mat(1, 2) * Mat(0, 3) + Mat(3, 1) * Mat(0, 2) * Mat(1, 3)
			- Mat(0, 1) * Mat(3, 2) * Mat(1, 3) - Mat(1, 1) * Mat(0, 2) * Mat(3, 3) + Mat(0, 1) * Mat(1, 2) * Mat(3, 3);

		Result.Mat(1, 2) = + Mat(3, 0) * Mat(1, 2) * Mat(0, 3) - Mat(1, 0) * Mat(3, 2) * Mat(0, 3) - Mat(3, 0) * Mat(0, 2) * Mat(1, 3)
			+ Mat(0, 0) * Mat(3, 2) * Mat(1, 3) + Mat(1, 0) * Mat(0, 2) * Mat(3, 3) - Mat(0, 0) * Mat(1, 2) * Mat(3, 3);

		Result.Mat(2, 2) = + Mat(1, 0) * Mat(3, 1) * Mat(0, 3) - Mat(3, 0) * Mat(1, 1) * Mat(0, 3) + Mat(3, 0) * Mat(0, 1) * Mat(1, 3)
			- Mat(0, 0) * Mat(3, 1) * Mat(1, 3) - Mat(1, 0) * Mat(0, 1) * Mat(3, 3) + Mat(0, 0) * Mat(1, 1) * Mat(3, 3);

		Result.Mat(3, 2) = + Mat(3, 0) * Mat(1, 1) * Mat(0, 2) - Mat(1, 0) * Mat(3, 1) * Mat(0, 2) - Mat(3, 0) * Mat(0, 1) * Mat(1, 2)
			+ Mat(0, 0) * Mat(3, 1) * Mat(1, 2) + Mat(1, 0) * Mat(0, 1) * Mat(3, 2) - Mat(0, 0) * Mat(1, 1) * Mat(3, 2);

		Result.Mat(0, 3) = + Mat(2, 1) * Mat(1, 2) * Mat(0, 3) - Mat(1, 1) * Mat(2, 2) * Mat(0, 3) - Mat(2, 1) * Mat(0, 2) * Mat(1, 3)
			+ Mat(0, 1) * Mat(2, 2) * Mat(1, 3) + Mat(1, 1) * Mat(0, 2) * Mat(2, 3) - Mat(0, 1) * Mat(1, 2) * Mat(2, 3);

		Result.Mat(1, 3) = + Mat(1, 0) * Mat(2, 2) * Mat(0, 3) - Mat(2, 0) * Mat(1, 2) * Mat(0, 3) + Mat(2, 0) * Mat(0, 2) * Mat(1, 3)
			- Mat(0, 0) * Mat(2, 2) * Mat(1, 3) - Mat(1, 0) * Mat(0, 2) * Mat(2, 3) + Mat(0, 0) * Mat(1, 2) * Mat(2, 3);

		Result.Mat(2, 3) = + Mat(2, 0) * Mat(1, 1) * Mat(0, 3) - Mat(1, 0) * Mat(2, 1) * Mat(0, 3) - Mat(2, 0) * Mat(0, 1) * Mat(1, 3)
			+ Mat(0, 0) * Mat(2, 1) * Mat(1, 3) + Mat(1, 0) * Mat(0, 1) * Mat(2, 3) - Mat(0, 0) * Mat(1, 1) * Mat(2, 3);

		Result.Mat(3, 3) = + Mat(1, 0) * Mat(2, 1) * Mat(0, 2) - Mat(2, 0) * Mat(1, 1) * Mat(0, 2) + Mat(2, 0) * Mat(0, 1) * Mat(1, 2)
			- Mat(0, 0) * Mat(2, 1) * Mat(1, 2) - Mat(1, 0) * Mat(0, 1) * Mat(2, 2) + Mat(0, 0) * Mat(1, 1) * Mat(2, 2);

		return Result / Determinant();
	}

	Vector3 Matrix::Position()
	{
		return Vector3(this->Mat(3, 0), this->Mat(3, 1), this->Mat(3, 2));
	}

	Quaternion Matrix::Rotation()
	{
		float X = 0, Y = 0, Z = 0, W = 0;

		float TransRemainder = this->Mat(0, 0) + this->Mat(1, 1) + this->Mat(2, 2);

		if (TransRemainder > 0)
		{
			float Scale = std::sqrt(TransRemainder + 1.0f) * 2;
			W = 0.25f * Scale;
			X = (this->Mat(2, 1) - this->Mat(1, 2)) / Scale;
			Y = (this->Mat(0, 2) - this->Mat(2, 0)) / Scale;
			Z = (this->Mat(1, 0) - this->Mat(0, 1)) / Scale;
		}
		else if ((this->Mat(0, 0) > this->Mat(1, 1)) && (this->Mat(0, 0) > this->Mat(2, 2)))
		{
			float Scale = std::sqrt(1.0f + this->Mat(0, 0) - this->Mat(1, 1) - this->Mat(2, 2)) * 2;
			W = (this->Mat(2, 1) - this->Mat(1, 2)) / Scale;
			X = 0.25f * Scale;
			Y = (this->Mat(0, 1) + this->Mat(1, 0)) / Scale;
			Z = (this->Mat(0, 2) + this->Mat(2, 0)) / Scale;
		}
		else if (this->Mat(1, 1) > this->Mat(2, 2))
		{
			float Scale = std::sqrt(1.0f + this->Mat(1, 1) - this->Mat(0, 0) - this->Mat(2, 2)) * 2;
			W = (this->Mat(0, 2) - this->Mat(2, 0)) / Scale;
			X = (this->Mat(0, 1) + this->Mat(1, 0)) / Scale;
			Y = 0.25f * Scale;
			Z = (this->Mat(1, 2) + this->Mat(2, 1)) / Scale;
		}
		else
		{
			float Scale = std::sqrt(1.0f + this->Mat(2, 2) - this->Mat(0, 0) - this->Mat(1, 1)) * 2;
			W = (this->Mat(1, 0) - this->Mat(0, 1)) / Scale;
			X = (this->Mat(0, 2) + this->Mat(2, 0)) / Scale;
			Y = (this->Mat(1, 2) + this->Mat(2, 1)) / Scale;
			Z = 0.25f * Scale;
		}

		return Quaternion(X, Y, Z, W);
	}

	Vector3 Matrix::Scale()
	{
		auto Row1 = Vector3(this->Mat(0, 0), this->Mat(0, 1), this->Mat(0, 2));
		auto Row2 = Vector3(this->Mat(1, 0), this->Mat(1, 1), this->Mat(1, 2));
		auto Row3 = Vector3(this->Mat(2, 0), this->Mat(2, 1), this->Mat(2, 2));

		return Vector3(Row1.Length(), Row2.Length(), Row3.Length());
	}

	void Matrix::SetPosition(const Vector3& Value)
	{
		this->Mat(3, 0) = Value.X;
		this->Mat(3, 1) = Value.Y;
		this->Mat(3, 2) = Value.Z;
	}

	void Matrix::SetRotation(const Quaternion& Rotation)
	{
		auto Scale = this->Scale();
		auto& Result = *this;

		float XX = Rotation.X * Rotation.X;
		float XY = Rotation.X * Rotation.Y;
		float XZ = Rotation.X * Rotation.Z;
		float XW = Rotation.X * Rotation.W;

		float YY = Rotation.Y * Rotation.Y;
		float YZ = Rotation.Y * Rotation.Z;
		float YW = Rotation.Y * Rotation.W;

		float ZZ = Rotation.Z * Rotation.Z;
		float ZW = Rotation.Z * Rotation.W;

		Result.Mat(0, 0) = 1 - 2 * (YY + ZZ);
		Result.Mat(1, 0) = 2 * (XY - ZW);
		Result.Mat(2, 0) = 2 * (XZ + YW);

		Result.Mat(0, 1) = 2 * (XY + ZW);
		Result.Mat(1, 1) = 1 - 2 * (XX + ZZ);
		Result.Mat(2, 1) = 2 * (YZ - XW);

		Result.Mat(0, 2) = 2 * (XZ - YW);
		Result.Mat(1, 2) = 2 * (YZ + XW);
		Result.Mat(2, 2) = 1 - 2 * (XX + YY);

		Result.Mat(0, 3) = 0;
		Result.Mat(1, 3) = 0;
		Result.Mat(2, 3) = 0;
		Result.Mat(3, 3) = 1;

		this->SetScale(Scale);
	}

	void Matrix::SetScale(const Vector3& Scale)
	{
		// TODO: How do we scale a matrix / apply scale
	}

	Matrix Matrix::CreateFromQuaternion(const Quaternion& Rotation)
	{
		Matrix Result;

		float XX = Rotation.X * Rotation.X;
		float XY = Rotation.X * Rotation.Y;
		float XZ = Rotation.X * Rotation.Z;
		float XW = Rotation.X * Rotation.W;

		float YY = Rotation.Y * Rotation.Y;
		float YZ = Rotation.Y * Rotation.Z;
		float YW = Rotation.Y * Rotation.W;

		float ZZ = Rotation.Z * Rotation.Z;
		float ZW = Rotation.Z * Rotation.W;

		Result.Mat(0, 0) = 1 - 2 * (YY + ZZ);
		Result.Mat(1, 0) = 2 * (XY - ZW);
		Result.Mat(2, 0) = 2 * (XZ + YW);
		Result.Mat(3, 0) = 0;

		Result.Mat(0, 1) = 2 * (XY + ZW);
		Result.Mat(1, 1) = 1 - 2 * (XX + ZZ);
		Result.Mat(2, 1) = 2 * (YZ - XW);
		Result.Mat(3, 1) = 0;

		Result.Mat(0, 2) = 2 * (XZ - YW);
		Result.Mat(1, 2) = 2 * (YZ + XW);
		Result.Mat(2, 2) = 1 - 2 * (XX + YY);
		Result.Mat(3, 2) = 0;

		Result.Mat(0, 3) = 0;
		Result.Mat(1, 3) = 0;
		Result.Mat(2, 3) = 0;
		Result.Mat(3, 3) = 1;

		return Result;
	}

	Vector3 Matrix::TransformVector(const Vector3& Vector, const Matrix& Value)
	{
		Vector3 Result;

		Result.X = (Vector.X * Value.Mat(0, 0)) + (Vector.Y * Value.Mat(1, 0)) + (Vector.Z * Value.Mat(2, 0)) + Value.Mat(3, 0);
		Result.Y = (Vector.X * Value.Mat(0, 1)) + (Vector.Y * Value.Mat(1, 1)) + (Vector.Z * Value.Mat(2, 1)) + Value.Mat(3, 1);
		Result.Z = (Vector.X * Value.Mat(0, 2)) + (Vector.Y * Value.Mat(1, 2)) + (Vector.Z * Value.Mat(2, 2)) + Value.Mat(3, 2);

		return Result;
	}

	Matrix Matrix::CreateLookAt(const Vector3& From, const Vector3& To, const Vector3& RightVec)
	{
		Matrix Result;

		auto Z = (From - To).GetNormalized();
		auto X = RightVec.Cross(Z).GetNormalized();
		auto Y = Z.Cross(X);

		Result.Mat(0, 0) = X.X;
		Result.Mat(0, 1) = Y.X;
		Result.Mat(0, 2) = Z.X;
		Result.Mat(0, 3) = 0;

		Result.Mat(1, 0) = X.Y;
		Result.Mat(1, 1) = Y.Y;
		Result.Mat(1, 2) = Z.Y;
		Result.Mat(1, 3) = 0;

		Result.Mat(2, 0) = X.Z;
		Result.Mat(2, 1) = Y.Z;
		Result.Mat(2, 2) = Z.Z;
		Result.Mat(2, 3) = 0;

		Result.Mat(3, 0) = -((X.X * From.X) + (X.Y * From.Y) + (X.Z * From.Z));
		Result.Mat(3, 1) = -((Y.X * From.X) + (Y.Y * From.Y) + (Y.Z * From.Z));
		Result.Mat(3, 2) = -((Z.X * From.X) + (Z.Y * From.Y) + (Z.Z * From.Z));
		Result.Mat(3, 3) = 1;

		return Result;
	}

	Matrix Matrix::CreatePerspectiveFov(float Fov, float Aspect, float Near, float Far)
	{
		Matrix Result;

		auto Top = Near * tanf(.5f * MathHelper::DegreesToRadians(Fov));
		auto Bottom = -Top;
		auto Right = Top * Aspect;
		auto Left = -Right;

		auto X = 2.f * Near / (Right - Left);
		auto Y = 2.f * Near / (Top - Bottom);
		auto A = (Right + Left) / (Right - Left);
		auto B = (Top + Bottom) / (Top - Bottom);
		auto C = -(Far + Near) / (Far - Near);
		auto D = -(2.f * Far * Near) / (Far - Near);

		Result.Mat(0, 0) = X;
		Result.Mat(0, 1) = 0;
		Result.Mat(0, 2) = 0;
		Result.Mat(0, 3) = 0;

		Result.Mat(1, 0) = 0;
		Result.Mat(1, 1) = Y;
		Result.Mat(1, 2) = 0;
		Result.Mat(1, 3) = 0;

		Result.Mat(2, 0) = A;
		Result.Mat(2, 1) = B;
		Result.Mat(2, 2) = C;
		Result.Mat(2, 3) = -1;

		Result.Mat(3, 0) = 0;
		Result.Mat(3, 1) = 0;
		Result.Mat(3, 2) = D;
		Result.Mat(3, 3) = 0;

		return Result;
	}

	Matrix Matrix::CreateOrthographic(float Left, float Right, float Bottom, float Top, float Near, float Far)
	{
		Matrix Result;

		Result.Mat(0, 0) = (2.f / (Right - Left));
		Result.Mat(0, 1) = 0;
		Result.Mat(0, 2) = 0;
		Result.Mat(0, 3) = 0;

		Result.Mat(1, 0) = 0;
		Result.Mat(1, 1) = (2.f / (Top - Bottom));
		Result.Mat(1, 2) = 0;
		Result.Mat(1, 3) = 0;

		Result.Mat(2, 0) = 0;
		Result.Mat(2, 1) = 0;
		Result.Mat(2, 2) = (1.f / (Near - Far));
		Result.Mat(2, 3) = 0;

		Result.Mat(3, 0) = ((Left + Right) / (Left - Right));
		Result.Mat(3, 1) = ((Top + Bottom) / (Bottom - Top));
		Result.Mat(3, 2) = (Near / (Near - Far));
		Result.Mat(3, 3) = 1;

		return Result;
	}
}