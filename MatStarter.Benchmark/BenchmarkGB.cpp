#include <benchmark/benchmark.h>
#include "Maths/VecBatch.h"
#include <vector>
#include <random>

#include "Maths/ASM/ASM_Vec3.h"

// Données générées hors chronométrage

static Maths::Vec3f RandomVec3(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
{
    return { dist(rng), dist(rng), dist(rng) };
}

static void PrepareData(std::vector<Maths::Vec3f>& a, std::vector<Maths::Vec3f>& b,
    std::vector<float>& result, size_t count)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    a.resize(count);
    b.resize(count);
    result.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        a[i] = RandomVec3(rng, dist);
        b[i] = RandomVec3(rng, dist);
    }
}

static void PrepareNormalizeData(std::vector<Maths::Vec3f>& input, size_t count)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    input.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        input[i] = RandomVec3(rng, dist);
    }
}

// ============================================================
//  DotProduct AOS
// ============================================================

static void BM_DotProduct_AOS_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        Maths::Ref::DotProduct_AOS(a.data(), b.data(), result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DotProduct_AOS_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        Maths::SSE::DotProduct_AOS(a.data(), b.data(), result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
//  DotProduct SOA
// ============================================================

static void BM_DotProduct_SOA_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);
    auto soa_a = Maths::ConvertToSOA(a.data(), count);
    auto soa_b = Maths::ConvertToSOA(b.data(), count);

    for (auto _ : state)
    {
        Maths::Ref::DotProduct_SOA(soa_a, soa_b, result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DotProduct_SOA_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);
    auto soa_a = Maths::ConvertToSOA(a.data(), count);
    auto soa_b = Maths::ConvertToSOA(b.data(), count);

    for (auto _ : state)
    {
        Maths::SSE::DotProduct_SOA(soa_a, soa_b, result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
// Normalize AOS
// ============================================================

static void BM_Normalize_AOS_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> input;
    std::vector<Maths::Vec3f> output(count);
    PrepareNormalizeData(input, count);

    for (auto _ : state)
    {
        Maths::Ref::NormalizeBatch_AOS(input.data(), output.data(), count);
        benchmark::DoNotOptimize(output.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_Normalize_AOS_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> input;
    std::vector<Maths::Vec3f> output(count);
    PrepareNormalizeData(input, count);

    for (auto _ : state)
    {
        Maths::SSE::NormalizeBatch_AOS(input.data(), output.data(), count);
        benchmark::DoNotOptimize(output.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DotProduct_ASM(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        Maths::ASM_Vec3_DotProduct(a.data(), b.data(), result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}


// 3 tailles : petite (64), moyenne (4096), grande (1M)
BENCHMARK(BM_DotProduct_AOS_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20);
BENCHMARK(BM_DotProduct_AOS_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20);
BENCHMARK(BM_DotProduct_SOA_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20);
BENCHMARK(BM_DotProduct_SOA_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20);
BENCHMARK(BM_Normalize_AOS_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20);
BENCHMARK(BM_Normalize_AOS_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20);
BENCHMARK(BM_DotProduct_ASM)->Arg(64)->Arg(4096)->Arg(1 << 20);

BENCHMARK_MAIN();
