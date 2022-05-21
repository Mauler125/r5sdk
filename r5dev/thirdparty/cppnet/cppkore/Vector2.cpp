#include "stdafx.h"
#include "Vector2.h"

namespace Math
{
	Vector2::Vector2()
		: X(0), Y(0)
	{
	}

	Vector2::Vector2(float X, float Y)
		: X(X), Y(Y)
	{
	}

	float& Vector2::operator[](int Index)
	{
		if (Index == 0)
			return X;

		return Y;
	}

	Vector2 Vector2::operator+(const Vector2& Rhs) const
	{
		return Vector2(X + Rhs.X, Y + Rhs.Y);
	}

	Vector2 Vector2::operator-(const Vector2& Rhs) const
	{
		return Vector2(X - Rhs.X, Y - Rhs.Y);
	}

	Vector2 Vector2::operator*(const Vector2& Rhs) const
	{
		return Vector2(X*Rhs.X, Y*Rhs.Y);
	}

	Vector2 Vector2::operator/(const Vector2& Rhs) const
	{
		return Vector2(X / Rhs.X, Y / Rhs.Y);
	}

	Vector2& Vector2::operator+=(const Vector2& Rhs)
	{
		X += Rhs.X;
		Y += Rhs.Y;

		return *this;
	}

	Vector2& Vector2::operator-=(const Vector2& Rhs)
	{
		X -= Rhs.X;
		Y -= Rhs.Y;

		return *this;
	}

	Vector2& Vector2::operator*=(const Vector2& Rhs)
	{
		X *= Rhs.X;
		Y *= Rhs.Y;

		return *this;
	}

	Vector2& Vector2::operator/=(const Vector2& Rhs)
	{
		X /= Rhs.X;
		Y /= Rhs.Y;

		return *this;
	}

	Vector2 Vector2::operator+(const float Rhs) const
	{
		return Vector2(X + Rhs, Y + Rhs);
	}

	Vector2 Vector2::operator-(const float Rhs) const
	{
		return Vector2(X - Rhs, Y - Rhs);
	}

	Vector2 Vector2::operator*(const float Rhs) const
	{
		return Vector2(X * Rhs, Y * Rhs);
	}

	Vector2 Vector2::operator/(const float Rhs) const
	{
		return Vector2(X / Rhs, Y / Rhs);
	}

	Vector2& Vector2::operator+=(const float Rhs)
	{
		X += Rhs;
		Y += Rhs;

		return *this;
	}

	Vector2& Vector2::operator-=(const float Rhs)
	{
		X -= Rhs;
		Y -= Rhs;

		return *this;
	}

	Vector2& Vector2::operator*=(const float Rhs)
	{
		X *= Rhs;
		Y *= Rhs;

		return *this;
	}

	Vector2& Vector2::operator/=(const float Rhs)
	{
		X /= Rhs;
		Y /= Rhs;

		return *this;
	}

	bool Vector2::operator==(const Vector2& Rhs) const
	{
		return (std::abs(X - Rhs.X) < MathHelper::Epsilon) && (std::abs(Y - Rhs.Y) < MathHelper::Epsilon);
	}

	bool Vector2::operator!=(const Vector2& Rhs) const
	{
		return !(*this == Rhs);
	}

	Vector2 Vector2::operator-() const
	{
		return Vector2(-X, -Y);
	}

	float Vector2::Length() const
	{
		return (float)std::sqrt(X * X + Y * Y);
	}

	float Vector2::LengthSq() const
	{
		return X * X + Y * Y;
	}

	void Vector2::Normalize()
	{
		auto Length = this->Length();

		if (Length > 0.0f)
		{
			X /= Length;
			Y /= Length;
		}
	}

	Vector2 Vector2::GetNormalized() const
	{
		auto Length = this->Length();

		return (Length > 0.0f) ? Vector2(X / Length, Y / Length) : Vector2(X, Y);
	}

	Vector2 Vector2::Lerp(float Factor, const Vector2& Rhs) const
	{
		return (*this) + (Rhs - (*this)) * Factor;
	}

	Vector2 Vector2::Zero()
	{
		return Vector2();
	}

	Vector2 Vector2::One()
	{
		return Vector2(1.0f, 1.0f);
	}
}