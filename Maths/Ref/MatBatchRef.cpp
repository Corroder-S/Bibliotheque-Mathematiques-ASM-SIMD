#include "Maths/Mat4Batch.h"

namespace Maths::Ref
{
    void TransformPointBatch(const Matrix4x4<>& transform, const Vec3<>* points, Vec3<>* outPoints, std::size_t count)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            outPoints[i] = transform.TransformPoint(points[i]);
        }
    }
}
