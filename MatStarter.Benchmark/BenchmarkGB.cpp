#include <iostream>
#include <benchmark/benchmark.h>
#include "Maths/Vec3Batch.h"
#include "Maths/Mat4Batch.h"
#include "Maths/Matrix4x4.h"
#include "Maths/ASM/Vec3.h"
#include <vector>
#include <random>

static Maths::Vec3f RandomVec3(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
{
    return { dist(rng), dist(rng), dist(rng) };
}

static void PrepareData(std::vector<Maths::Vec3f>& a, std::vector<Maths::Vec3f>& b, std::vector<float>& result, size_t count)
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

static void PrepareData(std::vector<Maths::Matrix4x4<>>& a, std::vector<Maths::Matrix4x4<>>& b, size_t count)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    a.resize(count);
    b.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                a[i].values[r][c] = dist(rng);
                b[i].values[r][c] = dist(rng);
            }
        }
    }
}

static void PrepareNormalizeData(std::vector<Maths::Vec3f>& input, size_t count)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    input.resize(count);
    for (size_t i = 0; i < count; ++i)
        input[i] = RandomVec3(rng, dist);
}

static void PrepareTransformData(std::vector<Maths::Vec3f>& points, size_t count)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    points.resize(count);
    for (size_t i = 0; i < count; ++i)
        points[i] = RandomVec3(rng, dist);
}

// ============================================================
//  Conversion AoS -> SoA (coût isolé)
// ============================================================

static void BM_ConvertToSOA(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        auto soa_a = Maths::ConvertToSOA(a.data(), count);
        benchmark::DoNotOptimize(soa_a.x.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
//  DotProduct AOS
// ============================================================

static void BM_DotProductBatch_AOS_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        Maths::Ref::DotProductBatch_AOS(a.data(), b.data(), result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DotProductBatch_AOS_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        Maths::SSE::DotProductBatch_AOS(a.data(), b.data(), result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
//  DotProduct SOA
// ============================================================

static void BM_DotProductBatch_SOA_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);
    auto soa_a = Maths::ConvertToSOA(a.data(), count);
    auto soa_b = Maths::ConvertToSOA(b.data(), count);

    for (auto _ : state)
    {
        Maths::Ref::DotProductBatch_SOA(soa_a, soa_b, result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DotProductBatch_SOA_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);
    auto soa_a = Maths::ConvertToSOA(a.data(), count);
    auto soa_b = Maths::ConvertToSOA(b.data(), count);

    for (auto _ : state)
    {
        Maths::SSE::DotProductBatch_SOA(soa_a, soa_b, result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
// Normalize AOS
// ============================================================

static void BM_NormalizeBatch_AOS_Ref(benchmark::State& state)
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

static void BM_NormalizeBatch_AOS_SSE(benchmark::State& state)
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

// ============================================================
// DotProduct ASM
// ============================================================

static void BM_DotProductBatch_AOS_ASM(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);

    for (auto _ : state)
    {
        Maths::ASM::DotProductBatch_AOS(a.data(), b.data(), result.data(), count);
        benchmark::DoNotOptimize(result.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
// TransformPoint
// ============================================================

static void BM_TransformPointBatch_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    const Maths::Matrix4x4<> matrix = Maths::Matrix4x4<>::Translation(Maths::Vec3<>{10.f, 20.f, 30.f});
    std::vector<Maths::Vec3f> points, output(count);
    PrepareTransformData(points, count);

    for (auto _ : state)
    {
        Maths::Ref::TransformPointBatch(matrix, points.data(), output.data(), count);
        benchmark::DoNotOptimize(output.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_TransformPointBatch_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    const Maths::Matrix4x4<> matrix = Maths::Matrix4x4<>::Translation(Maths::Vec3<>{10.f, 20.f, 30.f});
    std::vector<Maths::Vec3f> points, output(count);
    PrepareTransformData(points, count);

    for (auto _ : state)
    {
        Maths::SSE::TransformPointBatch(matrix, points.data(), output.data(), count);
        benchmark::DoNotOptimize(output.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
// Multiply Matrix
// ============================================================

static void BM_MatMul_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Matrix4x4<>> a(count), b(count), out(count);
    PrepareData(a, b, count);
    for (auto _ : state)
    {
        Maths::Ref::MultiplyMatrixBatch(a.data(), b.data(), out.data(), count);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_MatMul_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Matrix4x4<>> a, b, out(count);
    PrepareData(a, b, count);

    for (auto _ : state)
    {
        Maths::SSE::MultiplyMatrixBatch(a.data(), b.data(), out.data(), count);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// ============================================================
// CrossProduct SOA
// ============================================================

static void BM_CrossBatch_Ref(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);
    auto soa_a = Maths::ConvertToSOA(a.data(), count);
    auto soa_b = Maths::ConvertToSOA(b.data(), count);
    std::vector<Maths::Vec3f> out(count);

    for (auto _ : state)
    {
        Maths::Ref::CrossBatch(soa_a, soa_b, out.data(), count);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

static void BM_CrossBatch_SSE(benchmark::State& state)
{
    const size_t count = static_cast<size_t>(state.range(0));
    std::vector<Maths::Vec3f> a, b;
    std::vector<float> result;
    PrepareData(a, b, result, count);
    auto soa_a = Maths::ConvertToSOA(a.data(), count);
    auto soa_b = Maths::ConvertToSOA(b.data(), count);
    std::vector<Maths::Vec3f> out(count);

    for (auto _ : state)
    {
        Maths::SSE::CrossProductBatch(soa_a, soa_b, out.data(), count);
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * count);
}

// 3 tailles : petite (64), moyenne (4096), grande (1M)
BENCHMARK(BM_ConvertToSOA)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProductBatch_AOS_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProductBatch_AOS_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProductBatch_SOA_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProductBatch_SOA_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_NormalizeBatch_AOS_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_NormalizeBatch_AOS_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProductBatch_AOS_ASM)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_TransformPointBatch_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_TransformPointBatch_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_CrossBatch_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_CrossBatch_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_MatMul_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_MatMul_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);

static std::string ChooseBenchmark() {
    while (true)
    {
        std::cout << "\n=== Benchmarks disponibles ===\n"
            << "1. DotProduct AoS (Ref + SSE)\n"
            << "2. DotProduct SoA (Ref + SSE)\n"
            << "3. Normalize AoS (Ref + SSE)\n"
            << "4. DotProduct ASM\n"
            << "5. TransformPoint (Ref + SSE)\n"
            << "6. CrossProduct SoA (Ref + SSE)\n"
            << "7. Conversion AoS -> SoA\n"
            << "8. Matrix Multiplication (Ref + SSE)\n"
            << "9. Tout lancer\n"
            << "0. Quitter\n"
            << "> ";

        int choice = 0;
        std::cin >> choice;

        if (!std::cin)
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Entree invalide.\n";
            continue;
        }

        switch (choice)
        {
        case 1: return "BM_DotProductBatch_AOS";
        case 2: return "BM_DotProductBatch_SOA";
        case 3: return "BM_NormalizeBatch_AOS";
        case 4: return "BM_DotProductBatch_AOS_ASM";
        case 5: return "BM_TransformPointBatch";
        case 6: return "BM_CrossBatch";
        case 7: return "BM_ConvertToSOA";
		case 8: return "BM_MatMul";
        case 9: return ".*";
        case 0: return "";
        default:
            std::cout << "Choix invalide.\n";
        }
    }
}

int main(int argc, char** argv)
{
    std::cout << "Compiler: MSVC " << _MSC_VER << "\n";
    std::cout << "Build: Release x64 /O2 /arch:SSE2\n\n";
    ::benchmark::Initialize(&argc, argv);
    while (true)
    {
        const std::string filter = ChooseBenchmark();
        if (filter.empty())
        {
            break;
        }

        ::benchmark::RunSpecifiedBenchmarks(filter);
    }
    ::benchmark::Shutdown();
    std::cout << "\nBenchmarks completed.\n";
    return 0;
}
