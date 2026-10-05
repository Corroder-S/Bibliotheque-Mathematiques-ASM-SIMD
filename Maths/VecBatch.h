#pragma once
#include "Maths/Vec3.h"
#include <vector>


namespace Maths
{
    struct Vec3fSoA
    {
        std::vector<float> x;
        std::vector<float> y;
        std::vector<float> z;
    };

    Vec3fSoA ConvertToSOA(const Vec3f* input, size_t count);

    namespace Ref
    {
        void DotProduct_AOS(const Vec3f* a, const Vec3f* b, float* result, size_t count);
        void DotProduct_SOA(const Vec3fSoA& a, const Vec3fSoA& b, float* result, size_t count);
        void NormalizeBatch_AOS(const Vec3f* input, Vec3f* output, size_t count);
    }

    namespace SSE
    {
        void DotProduct_AOS(const Vec3f* a, const Vec3f* b, float* result, size_t count);
        void DotProduct_SOA(const Vec3fSoA& a, const Vec3fSoA& b, float* result, size_t count);
        void NormalizeBatch_AOS(const Vec3f* input, Vec3f* output, size_t count);
    }

}