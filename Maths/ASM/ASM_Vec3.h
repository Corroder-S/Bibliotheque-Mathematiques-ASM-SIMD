#pragma once
#include "Maths/Vec3.h"
extern "C" void ASM_Vec3_DotProduct(const Maths::Vec3<float>* a, const Maths::Vec3<float>* b, float* result, size_t count);
namespace Maths
{
        inline void ASM_Vec3_DotProduct(const Vec3<float>* a, const Vec3<float>* b, float* result, size_t count)
        {
                ::ASM_Vec3_DotProduct(a, b, result, count);
        }
}