#pragma once

// References:
// http://reedbeta.com/blog/on-vector-math-libraries/#how-to-simd-and-how-not-to
// https://github.com/Microsoft/DirectXMath

template <typename T, int32 n>
struct Vector
{
    T _X[n];

    constexpr Vector &operator+=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] += rhs._X[i];
    }

    constexpr Vector &operator-=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] -= rhs._X[i];
    }

    constexpr Vector &operator*=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] *= rhs._X[i];
    }

    constexpr Vector &operator/=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] /= rhs._X[i];
    }

    constexpr Vector &operator+=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi += rhs;
    }

    constexpr Vector &operator-=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi -= rhs;
    }

    constexpr Vector &operator*=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi *= rhs;
    }

    constexpr Vector &operator/=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi /= rhs;
    }

    constexpr T operator[](int32 i) {
        return _X[i];
    }
};

template <int32 Size>
constexpr bool All(const Vector<bool, Size> &vector)
{
    bool result = true;
    for (int32 i = 0; i < Size; ++i)
        result &= vector[i];
    return result;
}

template <int32 Size>
constexpr bool Any(const Vector<bool, Size> &vector)
{
    bool result = false;
    for (int32 i = 0; i < Size; ++i)
        result |= vector[i];
    return result;
}

template <int32 Size>
constexpr bool Dot(const Vector<bool, Size> &rhs)
{
    bool result = false;
    for (int32 i = 0; i < Size; ++i)
        result |= rhs[i];
    return result;
}

template <typename T, int32 Width, int32 Height>
struct Matrix
{
    Vector<T, Width> _X[Height];

    constexpr Vector<T, Width> operator[](int32 i) {
        return _X[i];
    }
};

template <typename T> struct Vector<T, 2> {
    union {
        T _X[2];
        struct {
            T x, y;
        };
    };
};
template <typename T> struct Vector<T, 3> {
    union {
        T _X[3];
        struct {
            T x, y, z;
        };
    };
};
template <typename T> struct Vector<T, 4> {
    union {
        T _X[4];
        struct {
            T x, y, z, w;
        };
    };
};

using Vector2 = Vector<float32, 2>;
using Vector3 = Vector<float32, 3>;
using Vector4 = Vector<float32, 4>;

using Matrix2x2 = Matrix<float32, 2, 2>;
using Matrix3x3 = Matrix<float32, 3, 3>;
using Matrix4x4 = Matrix<float32, 4, 4>;