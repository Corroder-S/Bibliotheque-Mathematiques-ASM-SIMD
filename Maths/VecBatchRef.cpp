#include "VecBatch.h"

namespace Maths
{
    Vec3fSoA ConvertToSOA(const Vec3f* input, size_t count)
    {
        Vec3fSoA result;
        result.x.resize(count);
        result.y.resize(count);
        result.z.resize(count);

        for (size_t i = 0; i < count; ++i)
        {
            result.x[i] = input[i].x;
            result.y[i] = input[i].y;
            result.z[i] = input[i].z;
        }

        return result;
    }

    namespace Ref
    {
        void DotProductAOS(const Vec3f* a, const Vec3f* b, float* result, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                result[i] = a[i].Dot(b[i]);
            }
        }

        void DotProductSOA(const Vec3fSoA& a, const Vec3fSoA& b, float* result, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                result[i] = a.x[i] * b.x[i] + a.y[i] * b.y[i] + a.z[i] * b.z[i];
            }
        }
    }
    
}