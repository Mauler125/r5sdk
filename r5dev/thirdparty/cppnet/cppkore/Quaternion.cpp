#include "stdafx.h"
#include "Quaternion.h"

namespace Math
{
	Quaternion::Quaternion()
		: X(0), Y(0), Z(0), W(1)
	{
	}

	Quaternion::Quaternion(float X, float Y, float Z, float W)
		: X(X), Y(Y), Z(Z), W(W)
	{
	}

	float& Quaternion::operator[](int Index)
	{
		if (Index == 0)
			return X;
		else if (Index == 1)
			return Y;
		else if (Index == 2)
			return Z;

		return W;
	}

	Quaternion Quaternion::operator+(const Quaternion& Rhs) const
	{
		return Quaternion(X + Rhs.X, Y + Rhs.Y, Z + Rhs.Z, W + Rhs.W);
	}

	Quaternion Quaternion::operator-(const Quaternion& Rhs) const
	{
		return Quaternion(X - Rhs.X, Y - Rhs.Y, Z - Rhs.Z, W - Rhs.W);
	}

	Quaternion Quaternion::operator*(const Quaternion& Rhs) const
	{
		return Quaternion(
			W * Rhs.X + X * Rhs.W + Y * Rhs.Z - Z * Rhs.Y,
			W * Rhs.Y - X * Rhs.Z + Y * Rhs.W + Z * Rhs.X,
			W * Rhs.Z + X * Rhs.Y - Y * Rhs.X + Z * Rhs.W,
			W * Rhs.W - X * Rhs.X - Y * Rhs.Y - Z * Rhs.Z);
	}

	Quaternion& Quaternion::operator+=(const Quaternion& Rhs)
	{
		X += Rhs.X;
		Y += Rhs.Y;
		Z += Rhs.Z;
		W += Rhs.W;

		return *this;
	}

	Quaternion& Quaternion::operator-=(const Quaternion& Rhs)
	{
		X -= Rhs.X;
		Y -= Rhs.Y;
		Z -= Rhs.Z;
		W -= Rhs.W;

		return *this;
	}

	Quaternion& Quaternion::operator*=(const Quaternion& Rhs)
	{
		auto Result = (*this) * Rhs;

		X = Result.X;
		Y = Result.Y;
		Z = Result.Z;
		W = Result.W;

		return (*this);
	}

	bool Quaternion::operator==(const Quaternion& Rhs) const
	{
		return (std::abs(X - Rhs.X) < MathHelper::Epsilon) && (std::abs(Y - Rhs.Y) < MathHelper::Epsilon) && (std::abs(Z - Rhs.Z) < MathHelper::Epsilon) && (std::abs(W - Rhs.W) < MathHelper::Epsilon);
	}

	bool Quaternion::operator!=(const Quaternion& Rhs) const
	{
		return !(*this == Rhs);
	}

	Quaternion Quaternion::operator-() const
	{
		return Quaternion(-X, -Y, -Z, -W);
	}

	Quaternion Quaternion::operator~() const
	{
		return Quaternion(-X, -Y, -Z, W);
	}

	float Quaternion::Length() const
	{
		return (float)std::sqrt(X * X + Y * Y + Z * Z + W * W);
	}

	float Quaternion::LengthSq() const
	{
		return X * X + Y * Y + Z * Z + W * W;
	}

	void Quaternion::Normalize()
	{
		auto Length = this->Length();

		X /= Length;
		Y /= Length;
		Z /= Length;
		W /= Length;
	}

	Quaternion Quaternion::GetNormalized() const
	{
		auto Length = this->Length();

		return Quaternion(X / Length, Y / Length, Z / Length, W / Length);
	}

	Vector3 Quaternion::ToEulerAngles() const
	{
		float Matrix[4][4];

		float num = X * X + Y * Y + Z * Z + W * W;
		float num2 = 0;

		if (num > 0.0)
		{
			num2 = 2.0f / num;
		}

		float num3 = X * num2;
		float num4 = Y * num2;
		float num5 = Z * num2;
		float num6 = W * num3;
		float num7 = W * num4;
		float num8 = W * num5;
		float num9 = X * num3;
		float num10 = X * num4;
		float num11 = X * num5;
		float num12 = Y * num4;
		float num13 = Y * num5;
		float num14 = Z * num5;

		Matrix[0][0] = 1.0f - (num12 + num14);
		Matrix[0][1] = num10 - num8;
		Matrix[0][2] = num11 + num7;
		Matrix[1][0] = num10 + num8;
		Matrix[1][1] = 1.0f - (num9 + num14);
		Matrix[1][2] = num13 - num6;
		Matrix[2][0] = num11 - num7;
		Matrix[2][1] = num13 + num6;
		Matrix[2][2] = 1.0f - (num9 + num12);
		Matrix[3][3] = 1.0f;

		uint32_t i = 0, j = 1, k = 2;
		float TempX = 0, TempY = 0, TempZ = 0;

		float SqSum = std::sqrt(Matrix[i][i] * Matrix[i][i] + Matrix[j][i] * Matrix[j][i]);
		if (SqSum > 0.00016)	// This value seems to be better than Math::Epsilon
		{
			TempX = std::atan2(Matrix[k][j], Matrix[k][k]);
			TempY = std::atan2(-Matrix[k][i], SqSum);
			TempZ = std::atan2(Matrix[j][i], Matrix[i][i]);
		}
		else
		{
			TempX = std::atan2(-Matrix[j][k], Matrix[j][j]);
			TempY = std::atan2(-Matrix[k][i], SqSum);
			TempZ = 0.0;
		}

		return Vector3(MathHelper::RadiansToDegrees(TempX), MathHelper::RadiansToDegrees(TempY), MathHelper::RadiansToDegrees(TempZ));
	}

	Quaternion Quaternion::Inverse() const
	{
		auto LengthSq = this->LengthSq();
		auto HalfLength = 1.0f / LengthSq;

		return Quaternion(-X * HalfLength, -Y * HalfLength, -Z * HalfLength, W * HalfLength);
	}

	Quaternion Quaternion::Slerp(Quaternion& Rhs, float Time)
	{
		float cosTheta = W * Rhs.W + X * Rhs.X + Y * Rhs.Y + Z * Rhs.Z;
		float theta = (float)acosf(cosTheta);

		if (fabs(theta) < Math::MathHelper::Epsilon)
		{
			return *this;
		}
		else
		{
			Quaternion ret;
			float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
			if (fabs(sinTheta) < Math::MathHelper::Epsilon)
			{
				ret.W = 0.5f * W + 0.5f * Rhs.W;
				
				auto Tmp = Math::Vector3(X, Y, Z).Lerp(0.5, Math::Vector3(Rhs.X, Rhs.Y, Rhs.Z));
				ret.X = Tmp.X;
				ret.Y = Tmp.Y;
				ret.Z = Tmp.Z;
			}
			else
			{
				float rA = sinf((1.0f - Time) * theta) / sinTheta;
				float rB = sinf(Time * theta) / sinTheta;

				ret.W = W * rA + Rhs.W * rB;
				ret.X = X * rA + Rhs.X * rB;
				ret.Y = Y * rA + Rhs.Y * rB;
				ret.Z = Z * rA + Rhs.Z * rB;
			}

			return ret;
		}
	}

	Quaternion Quaternion::FromEulerAngles(float X, float Y, float Z)
	{
		return FromAxisRotation(Vector3(1, 0, 0), X) * FromAxisRotation(Vector3(0, 1, 0), Y) * FromAxisRotation(Vector3(0, 0, 1), Z);
	}

	Quaternion Quaternion::FromAxisRotation(Vector3 Axis, float Angle)
	{
		auto Radians = MathHelper::DegreesToRadians(Angle);
		auto AngleScale = std::sin(Radians / 2.0f);
		auto QuaternionScale = std::cos(Radians / 2.0f);

		auto AngleResult = Axis * AngleScale;

		return Quaternion(AngleResult.X, AngleResult.Y, AngleResult.Z, QuaternionScale);
	}

	Quaternion Quaternion::Identity()
	{
		return Quaternion();
	}
}