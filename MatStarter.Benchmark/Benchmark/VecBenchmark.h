#pragma once

#include "Maths/VecBatch.h"
#include "Benchmark/Benchmark.h"
#include "Benchmark/BenchmarkData.h"

inline Benchmark::Result Ref_DotProduct(BenchmarkData& data)
{
    return Benchmark::Run([&]
    {
            Maths::Ref::DotProduct_AOS(data.vec3A.data(), data.vec3B.data(), data.floatOut.data(), data.vec3A.size());
            return static_cast<double>(data.floatOut[0]);
    });
}

inline Benchmark::Result SSE_DotProduct(BenchmarkData& data)
{
    return Benchmark::Run([&]
    {
            Maths::SSE::DotProduct_AOS(data.vec3A.data(), data.vec3B.data(), data.floatOut.data(), data.vec3A.size());
            return static_cast<double>(data.floatOut[0]);
    });
}