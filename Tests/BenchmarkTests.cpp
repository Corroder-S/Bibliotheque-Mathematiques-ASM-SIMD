#include "TestHelpers.h"
#include "Perf/PerfRun.h"

TEST(BenchmarkTests, WarmupAndSamplesInvokeWorkExactlyOncePerBatch)
{
    int calls = 0;
    const auto result = Benchmark::Run([&] { ++calls; return 2.0; }, {2,5});
    EXPECT_TRUE(calls == 7);
    EXPECT_TRUE(result.milliseconds.size() == 5);
    TestHelpers::Near(14.0, result.checksum);
}

TEST(BenchmarkTests, StatisticsMatchRecordedSamplesWithoutTimingThresholds)
{
    int sequence = 0;
    const auto result = Benchmark::Run([&] { return ++sequence; }, {0,4});
    auto samples = result.milliseconds;
    std::sort(samples.begin(), samples.end());
    EXPECT_TRUE(samples.front() >= 0);
    TestHelpers::Near(samples.front(), result.minimumMs);
    TestHelpers::Near(samples.back(), result.maximumMs);
    TestHelpers::Near((samples[1] + samples[2]) / 2, result.medianMs);
    TestHelpers::Near(10.0, result.checksum);
}

TEST(BenchmarkTests, SingleSampleIsItsOwnMedian)
{
    const auto result = Benchmark::Run([] { return 3.0; }, {0,1});
    EXPECT_TRUE(result.minimumMs == result.medianMs);
    EXPECT_TRUE(result.maximumMs == result.medianMs);
}

TEST(BenchmarkTests, EmptySamplingIsRejectedBeforeRunningWork)
{
    bool called = false;
    EXPECT_THROW(
        (void)Benchmark::Run([&] { called = true; return 0; }, {1,0}),
        std::invalid_argument
    );
    EXPECT_FALSE(called);
}

TEST(BenchmarkTests, WorkExceptionsAreNotHidden)
{
    EXPECT_THROW(
        (void)Benchmark::Run([]() -> double { throw std::domain_error("work failed"); }, {0,1}),
        std::domain_error
    );
}

TEST(BenchmarkTests, NonFiniteChecksumIsRejected)
{
    EXPECT_THROW(
        (void)Benchmark::Run([] { return std::numeric_limits<double>::quiet_NaN(); }, {0,1}),
        std::runtime_error
    );
}
