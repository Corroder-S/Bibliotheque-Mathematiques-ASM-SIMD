#include <iostream>
#include <benchmark/benchmark.h>
#include "Maths/VecBatch.h"
#include <vector>
#include <random>

#include "Maths/MatBatch.h"
#include "Maths/Matrix4x4.h"
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
    std::vector<float> result; // inutile ici mais garde PrepareData tel quel
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

// ============================================================
// TransformPoint (matrice 4x4 * points 3D)
// ============================================================

static void BM_TransformPoint_Ref(benchmark::State& state)
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

static void BM_TransformPoint_SSE(benchmark::State& state)
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


// 3 tailles : petite (64), moyenne (4096), grande (1M)
BENCHMARK(BM_ConvertToSOA)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProduct_AOS_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProduct_AOS_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProduct_SOA_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProduct_SOA_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_Normalize_AOS_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_Normalize_AOS_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_DotProduct_ASM)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_TransformPoint_Ref)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);
BENCHMARK(BM_TransformPoint_SSE)->Arg(64)->Arg(4096)->Arg(1 << 20)->Repetitions(5);

static std::string ChooseBenchmark() {
    while (true)
    {
        std::cout << "\n=== Benchmarks disponibles ===\n"
            << "1. DotProduct AoS (Ref + SSE)\n"
            << "2. DotProduct SoA (Ref + SSE)\n"
            << "3. Normalize AoS (Ref + SSE)\n"
            << "4. DotProduct ASM\n"
            << "5. TransformPoint (Ref + SSE)\n"
            << "6. Conversion AoS -> SoA\n"
            << "7. Tout lancer\n"
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
        case 1: return "BM_DotProduct_AOS";
        case 2: return "BM_DotProduct_SOA";
        case 3: return "BM_Normalize_AOS";
        case 4: return "BM_DotProduct_ASM";
        case 5: return "BM_TransformPoint";
        case 6: return "BM_ConvertToSOA";
        case 7: return ".*";
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

