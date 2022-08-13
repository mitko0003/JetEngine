#pragma once

#include "Core/Misc/Utility.h"

// References:
// http://reedbeta.com/blog/on-vector-math-libraries/#how-to-simd-and-how-not-to
// https://github.com/Microsoft/DirectXMath

template <typename T, int32 _n>
struct Vector
{
    static constexpr auto n = _n;
    using TVector = Vector<T, n>;

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

    template <CIntegral TIndex>
    constexpr T &operator[](TIndex i) {
        return _X[i];
    }
    template <CIntegral TIndex>
    constexpr const T &operator[](TIndex i) const {
        return _X[i];
    }

    static constexpr TVector Zero()
    {
        TVector result;
        FillN(result._X, n, T(0));
        return result;
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

template <typename T, int32 _Width, int32 _Height>
struct Matrix
{
    static constexpr auto Width = _Width;
    static constexpr auto Height = _Height;
    using TMatrix = Matrix<T, Width, Height>;
    using TRow = Vector<T, Width>;

    TRow _X[Height];

    template <CIntegral TIndex>
    constexpr TRow &operator[](TIndex i) {
        return _X[i];
    }
    template <CIntegral TIndex>
    constexpr const TRow &operator[](TIndex i) const {
        return _X[i];
    }

    static constexpr TMatrix Zero()
    {
        TMatrix result;
        for (int32 i = 0; i < Height; ++i)
            result[i] = TRow::Zero();
        return result;
    }

    static constexpr TMatrix Identity()
    {
        TMatrix result = Zero();
        for (int32 d = 0; d < Min(Width, Height); ++d)
            result[d][d] = T(1);
        return result;
    }
};

template <typename T>
struct Vector<T, 2>
{
    static constexpr auto n = 2;
    using TVector = Vector<T, n>;

    Vector(T x = {}, T y = {}) :
        x(x), y(y) {}

    union {
        T _X[2];
        struct {
            T x, y;
        };
    };

    template <CIntegral TIndex>
    constexpr T &operator[](TIndex i) {
        return _X[i];
    }
    template <CIntegral TIndex>
    constexpr const T &operator[](TIndex i) const {
        return _X[i];
    }

    static constexpr TVector Zero()
    {
        TVector result;
        FillN(result._X, n, T(0));
        return result;
    }
};

template <typename T>
struct Vector<T, 3>
{
    static constexpr auto n = 3;
    using TVector = Vector<T, n>;

    Vector(T x = {}, T y = {}, T z = {}) :
        x(x), y(y), z(z) {}

    union {
        T _X[3];
        struct {
            T x, y, z;
        };
    };

    template <CIntegral TIndex>
    constexpr T &operator[](TIndex i) {
        return _X[i];
    }
    template <CIntegral TIndex>
    constexpr const T &operator[](TIndex i) const {
        return _X[i];
    }

    static constexpr TVector Zero()
    {
        TVector result;
        FillN(result._X, n, T(0));
        return result;
    }
};

template <typename T>
struct Vector<T, 4>
{
    static constexpr auto n = 4;
    using TVector = Vector<T, n>;

    Vector(T x = {}, T y = {}, T z = {}, T w = {}) :
        x(x), y(y), z(z), w(w) {}

    union {
        T _X[4];
        struct {
            T x, y, z, w;
        };
    };

    template <CIntegral TIndex>
    constexpr T &operator[](TIndex i) {
        return _X[i];
    }
    template <CIntegral TIndex>
    constexpr const T &operator[](TIndex i) const {
        return _X[i];
    }

    static constexpr TVector Zero()
    {
        TVector result;
        FillN(result._X, n, T(0));
        return result;
    }
};

using Vector2 = Vector<float32, 2>;
using Vector3 = Vector<float32, 3>;
using Vector4 = Vector<float32, 4>;

using Vector2i = Vector<int32, 2>;
using Vector3i = Vector<int32, 3>;
using Vector4i = Vector<int32, 4>;

using Matrix2x2 = Matrix<float32, 2, 2>;
using Matrix3x3 = Matrix<float32, 3, 3>;
using Matrix4x4 = Matrix<float32, 4, 4>;

Matrix4x4 BuildPerspectiveMatrix(float fovY, float nearZ, float farZ);