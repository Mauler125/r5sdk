#pragma once

class Vector3;
class QAngle;
typedef float vec_t;
typedef float vec3_t[3];

/*-----------------------------------------------------------------------------
 * _vector.h
 *-----------------------------------------------------------------------------*/

class Vector3 // TODO [ AMOS ]: Reverse class
{
public:
    // Members
    vec_t x, // 0x0000
        y, // 0x0004
        z; // 0x0008

    Vector3()
    {
        Invalidate();
    }

    inline Vector3(vec_t X, vec_t Y, vec_t Z)
    {
        x = X; y = Y; z = Z;
    }

    // Equality

    __forceinline bool operator==(const Vector3& src) const
    {
        return (src.x == x) && (src.y == y) && (src.z == z);
    }

    __forceinline bool operator!=(const Vector3& src) const
    {
        return (src.x != x) || (src.y != y) || (src.z != z);
    }

    // Arithmetic operations

    __forceinline Vector3& operator=(const Vector3& vOther)
    {
        x = vOther.x; y = vOther.y; z = vOther.z;
        return *this;
    }

    __forceinline Vector3 operator-(void) const
    {
        return Vector3(-x, -y, -z);
    }

    __forceinline Vector3 operator+(const Vector3& v) const
    {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }

    __forceinline Vector3 operator-(const Vector3& v) const
    {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }

    __forceinline Vector3 operator*(const Vector3& v) const
    {
        return Vector3(x * v.x, y * v.y, z * v.z);
    }

    __forceinline Vector3 operator/(const Vector3& v) const
    {
        return Vector3(x / v.x, y / v.y, z / v.z);
    }

    __forceinline Vector3 operator+(float fl) const
    {
        return Vector3(x + fl, y + fl, z + fl);
    }

    __forceinline Vector3 operator-(float fl) const
    {
        return Vector3(x - fl, y - fl, z - fl);
    }

    __forceinline Vector3 operator/(float fl) const
    {
        return Vector3(x / fl, y / fl, z / fl);
    }

    __forceinline Vector3 operator*(float fl) const
    {
        return Vector3(x * fl, y * fl, z * fl);
    }

    __forceinline Vector3& operator+=(const Vector3& v)
    {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }

    __forceinline Vector3& operator-=(const Vector3& v)
    {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }

    __forceinline Vector3& operator*=(const Vector3& v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    __forceinline  Vector3& operator/=(const Vector3& v)
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }

    __forceinline Vector3& operator*=(float fl)
    {
        x *= fl;
        y *= fl;
        z *= fl;
        return *this;
    }

    __forceinline Vector3& operator+=(float fl)
    {
        x += fl;
        y += fl;
        z += fl;
        return *this;
    }

    __forceinline Vector3& operator/=(float fl)
    {
        x /= fl;
        y /= fl;
        z /= fl;
        return *this;
    }

    __forceinline Vector3& operator-=(float fl)
    {
        x -= fl;
        y -= fl;
        z -= fl;
        return *this;
    }

    // Vanity functions

    inline void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f)
    {
        x = ix; y = iy; z = iz;
    }

    inline bool isFinite() const
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }

    inline void Invalidate()
    {
        x = y = z = std::numeric_limits<float>::infinity();
    }

    inline void Zero()
    {
        x = y = z = 0.0f;
    }

    inline void Vector3CrossProduct(const Vector3& a, const Vector3& b, Vector3& result)
    {
        result.x = a.y * b.z - a.z * b.y;
        result.y = a.z * b.x - a.x * b.z;
        result.z = a.x * b.y - a.y * b.x;
    }

    inline Vector3 Cross(const Vector3& vOther)
    {
        Vector3 res;
        Vector3CrossProduct(*this, vOther, res);
        return res;
    }

    inline void NormalizeInPlace()
    {
        *this = Normalized();
    }

    inline Vector3 Normalized() const
    {
        const float r = 1.0f / (this->Length() + FLT_EPSILON);
        return { x * r, y * r, z * r };
    }

    inline float Normalize() const
    {
        return 1.0f / (this->Length() + FLT_EPSILON);
    }

    inline float DistTo(const Vector3& vOther) const
    {
        Vector3 delta;

        delta.x = x - vOther.x;
        delta.y = y - vOther.y;
        delta.z = z - vOther.z;

        return delta.Length();
    }

    inline float DistTo2D(const Vector3& vOther) const
    {
        Vector3 delta;

        delta.x = x - vOther.x;
        delta.y = y - vOther.y;

        return delta.Length2D();
    }

    inline float Area()
    {
        return x * x + y * y + z * z;
    }

    inline float Magnitude()
    {
        return FastSqrt(Area());
    }

    inline float AngleTo(Vector3 vOther)
    {
        return this->Dot(vOther) / (Magnitude() * vOther.Magnitude());
    }

    inline float DistToSqr(const Vector3& vOther) const
    {
        Vector3 delta;

        delta.x = x - vOther.x;
        delta.y = y - vOther.y;
        delta.z = z - vOther.z;

        return delta.LengthSqr();
    }

    inline float Dot(const Vector3& vOther) const
    {
        return (x * vOther.x + y * vOther.y + z * vOther.z);
    }

    float Length() const
    {
        return FastSqrt(x * x + y * y + z * z);
    }

    inline float LengthSqr(void) const
    {
        return (x * x + y * y + z * z);
    }

    inline float Length2D() const
    {
        return FastSqrt(x * x + y * y);
    }

    inline bool IsBetween(const Vector3& vMin, const Vector3& vMax)
    {
        return ((vMin.x <= x && x <= vMax.x) &&
            (vMin.y <= y && y <= vMax.y) &&
            (vMin.z <= z && z <= vMax.z));
    }

    static __forceinline float __vectorcall FastSqrt(float x)
    {
        __m128 root = _mm_sqrt_ss(_mm_load_ss(&x));
        return *(reinterpret_cast<float*>(&root));
    }
};

class QAngle // TODO [ AMOS ]: Reverse class
{
public:
    // Members
    vec_t x, // 0x0000
          y, // 0x0004
          z; // 0x0008

    QAngle()
    {
        Invalidate();
    }

    inline QAngle(vec_t X, vec_t Y, vec_t Z)
    {
        x = X; y = Y; z = Z;
    }

    inline void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f)
    {
        x = ix; y = iy; z = iz;
    }

    inline void Clear()
    {
        x = y = z = 0.0f;
    }

    inline bool isFinite() const
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }

    inline void Invalidate()
    {
        x = y = z = std::numeric_limits<float>::infinity();
    }
};