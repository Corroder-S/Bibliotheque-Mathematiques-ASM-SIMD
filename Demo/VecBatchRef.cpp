#include "VecBatch.h"

namespace Maths
{
    void DotProduct_Reference(const Vec3f* a, const Vec3f* b, float* result, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            result[i] = a[i].Dot(b[i]);
        }
    }
}