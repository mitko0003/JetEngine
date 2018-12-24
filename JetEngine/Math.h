#pragma once

// References:
// http://reedbeta.com/blog/on-vector-math-libraries/#how-to-simd-and-how-not-to
// https://github.com/Microsoft/DirectXMath

template <typename T, int32 n>
struct Vector
{
    T _X[n];

    Vector &operator+=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] += rhs._X[i];
    }

    Vector &operator-=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] -= rhs._X[i];
    }

    Vector &operator*=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] *= rhs._X[i];
    }

    Vector &operator/=(const Vector &rhs)
    {
        for (int32 i = 0; i < n; ++i)
            _X[i] /= rhs._X[i];
    }

    Vector &operator+=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi += rhs;
    }

    Vector &operator-=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi -= rhs;
    }

    Vector &operator*=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi *= rhs;
    }

    Vector &operator/=(const T &rhs)
    {
        for (auto &Xi : _X)
            Xi /= rhs;
    }

    T operator[](int32 i) {
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