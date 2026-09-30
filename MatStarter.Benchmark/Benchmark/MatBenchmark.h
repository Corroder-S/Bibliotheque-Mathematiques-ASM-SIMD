#pragma once

#include "Maths/MatBatch.h"
#include "Benchmark/Benchmark.h"
#include "Benchmark/BenchmarkData.h"


inline Benchmark::Result Ref_TransformPointBatch(BenchmarkData& data)
{
    return Benchmark::Run([&]
    {
        Maths::Ref::TransformPointBatch(data.matrix4x4, data.vec3A.data(), data.vec3Out.data(), data.vec3A.size());
        return static_cast<double>(data.vec3Out[0].x);
    });
}
inline Benchmark::Result SSE_TransformPointBatch(BenchmarkData& data)
{
    return Benchmark::Run([&]
    {
        Maths::SSE::TransformPointBatch(data.matrix4x4, data.vec3A.data(), data.vec3Out.data(), data.vec3A.size());
        return static_cast<double>(data.vec3Out[0].x);
    });
}