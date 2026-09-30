#include "VecBatch.h"

namespace Maths
{
    namespace SSE
    {
        void DotProductAoS(const Vec3f* a, const Vec3f* b, float* result, size_t count)
        {
            size_t i = 0;
            const size_t width = 4;

            for (; i + width <= count; i += width)
            {
                __m128 ax = _mm_set_ps(a[i + 3].x, a[i + 2].x, a[i + 1].x, a[i + 0].x);
                __m128 ay = _mm_set_ps(a[i + 3].y, a[i + 2].y, a[i + 1].y, a[i + 0].y);
                __m128 az = _mm_set_ps(a[i + 3].z, a[i + 2].z, a[i + 1].z, a[i + 0].z);

                __m128 bx = _mm_set_ps(b[i + 3].x, b[i + 2].x, b[i + 1].x, b[i + 0].x);
                __m128 by = _mm_set_ps(b[i + 3].y, b[i + 2].y, b[i + 1].y, b[i + 0].y);
                __m128 bz = _mm_set_ps(b[i + 3].z, b[i + 2].z, b[i + 1].z, b[i + 0].z);

                __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ax, bx), _mm_mul_ps(ay, by)), _mm_mul_ps(az, bz));

                _mm_storeu_ps(&result[i], dot);
            }

            for (; i < count; ++i)
            {
                result[i] = a[i].Dot(b[i]);
            }

        }

        void DotProductSoA(const Vec3fSoA& a, const Vec3fSoA& b, float* result, size_t count)
        {
            size_t i = 0;
            const size_t simdWidth = 4;

            for (; i + simdWidth <= count; i += simdWidth)
            {
                __m128 ax = _mm_loadu_ps(&a.x[i]);
                __m128 ay = _mm_loadu_ps(&a.y[i]);
                __m128 az = _mm_loadu_ps(&a.z[i]);

                __m128 bx = _mm_loadu_ps(&b.x[i]);
                __m128 by = _mm_loadu_ps(&b.y[i]);
                __m128 bz = _mm_loadu_ps(&b.z[i]);

                __m128 dot = _mm_add_ps(_mm_add_ps(_mm_mul_ps(ax, bx), _mm_mul_ps(ay, by)), _mm_mul_ps(az, bz));

                _mm_storeu_ps(&result[i], dot);
            }

            for (; i < count; ++i)
            {
                result[i] = a.x[i] * b.x[i] + a.y[i] * b.y[i] + a.z[i] * b.z[i];
            }
        }
    }
}