#pragma once
#include "Maths/Vec3.h"

namespace Maths
{
    void DotProduct_Reference(const Vec3f* a, const Vec3f* b, float* out, size_t count);
    void DotProduct_SIMD(const Vec3f* a, const Vec3f* b, float* out, size_t count);
}