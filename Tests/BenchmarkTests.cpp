#include "TestHelpers.h"
#include "Perf/PerfRun.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MathStarterTests
{
    TEST_CLASS(BenchmarkTests)
    {
    public:
        TEST_METHOD(WarmupAndSamplesInvokeWorkExactlyOncePerBatch)
        {
            int calls = 0;
            const auto result = Benchmark::Run([&] { ++calls; return 2.0; }, {2,5});
            Assert::IsTrue(calls == 7);
            Assert::IsTrue(result.milliseconds.size() == 5);
            TestHelpers::Near(14.0, result.checksum);
        }

        TEST_METHOD(StatisticsMatchRecordedSamplesWithoutTimingThresholds)
        {
            int sequence = 0;
            const auto result = Benchmark::Run([&] { return ++sequence; }, {0,4});
            auto samples = result.milliseconds;
            std::sort(samples.begin(), samples.end());
            Assert::IsTrue(samples.front() >= 0);
            TestHelpers::Near(samples.front(), result.minimumMs);
            TestHelpers::Near(samples.back(), result.maximumMs);
            TestHelpers::Near((samples[1] + samples[2]) / 2, result.medianMs);
            TestHelpers::Near(10.0, result.checksum);
        }

        TEST_METHOD(SingleSampleIsItsOwnMedian)
        {
            const auto result = Benchmark::Run([] { return 3.0; }, {0,1});
            Assert::IsTrue(result.minimumMs == result.medianMs);
            Assert::IsTrue(result.maximumMs == result.medianMs);
        }

        TEST_METHOD(EmptySamplingIsRejectedBeforeRunningWork)
        {
            bool called = false;
            Assert::ExpectException<std::invalid_argument>([&]
            {
                (void)Benchmark::Run([&] { called = true; return 0; }, {1,0});
            });
            Assert::IsFalse(called);
        }

        TEST_METHOD(WorkExceptionsAreNotHidden)
        {
            Assert::ExpectException<std::domain_error>([]
            {
                (void)Benchmark::Run([]() -> double
                {
                    throw std::domain_error("work failed");
                }, {0,1});
            });
        }

        TEST_METHOD(NonFiniteChecksumIsRejected)
        {
            Assert::ExpectException<std::runtime_error>([]
            {
                (void)Benchmark::Run([] { return std::numeric_limits<double>::quiet_NaN(); }, {0,1});
            });
        }
    };
}
