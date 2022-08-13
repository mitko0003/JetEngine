#include "Core/Math/Math.h"

Matrix4x4 BuildPerspectiveMatrix(float fovY, float nearZ, float farZ)
{
    auto result = Matrix4x4::Identity();
    result[3][3] = 0.0f;
    return result;
}