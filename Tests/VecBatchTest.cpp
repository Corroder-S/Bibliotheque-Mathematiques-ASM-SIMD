#include "TestHelpers.h"
#include "Maths/Vec3.h"
#include "Maths/VecBatch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
    void BuildKnownData(std::vector<Maths::Vec3f>& a, std::vector<Maths::Vec3f>& b, size_t count)
    {
        a.resize(count);
        b.resize(count);
        for (size_t i = 0; i < count; ++i)
        {
            a[i] = Maths::Vec3f{ static_cast<float>(i + 1), static_cast<float>(i + 2), static_cast<float>(i + 3) };
            b[i] = Maths::Vec3f{ static_cast<float>(i + 2), static_cast<float>(i + 3), static_cast<float>(i + 4) };
        }
    }

    void Ref_DotProductMatchesKnownValues()
    {
        const Maths::Vec3f a[] = { {1,2,3}, {2,3,4} };
        const Maths::Vec3f b[] = { {2,3,4}, {1,0,0} };
        float out[2] = {};

        Maths::Ref::DotProduct_AOS(a, b, out, 2);

        TestHelpers::Near(20.0f, out[0]);
        TestHelpers::Near(2.0f, out[1]);
    }

    void DotProduct_HandlesEmptyBatch()
    {
        std::vector<Maths::Vec3f> a, b;
        std::vector<float> out;

        Maths::Ref::DotProduct_AOS(a.data(), b.data(), out.data(), 0);
        Maths::SSE::DotProduct_AOS(a.data(), b.data(), out.data(), 0);
    }

    void RefAndSSE_AoS_GiveSameResult(size_t count)
    {
        std::vector<Maths::Vec3f> a, b;
        BuildKnownData(a, b, count);

        std::vector<float> outRef(count);
        std::vector<float> outSse(count);

        Maths::Ref::DotProduct_AOS(a.data(), b.data(), outRef.data(), count);
        Maths::SSE::DotProduct_AOS(a.data(), b.data(), outSse.data(), count);

        for (size_t i = 0; i < count; ++i)
        {
            TestHelpers::Near(outRef[i], outSse[i]);
        }
    }

    void RefAndSSE_SoA_GiveSameResult(size_t count)
    {
        std::vector<Maths::Vec3f> a, b;
        BuildKnownData(a, b, count);

        Maths::Vec3fSoA soaA = Maths::ConvertToSOA(a.data(), count);
        Maths::Vec3fSoA soaB = Maths::ConvertToSOA(b.data(), count);

        std::vector<float> outRef(count);
        std::vector<float> outSoa(count);

        Maths::Ref::DotProduct_SOA(soaA, soaB, outRef.data(), count);
        Maths::SSE::DotProduct_SOA(soaA, soaB, outSoa.data(), count);

        for (size_t i = 0; i < count; ++i)
        {
            TestHelpers::Near(outRef[i], outSoa[i]);
        }
    }
}

namespace MathStarterTests
{
    TEST_CLASS(VecBatchTests)
    {
    public:
        TEST_METHOD(Ref_DotProductMatchesKnownValues)
        {
            ::Ref_DotProductMatchesKnownValues();
        }

        TEST_METHOD(DotProduct_HandlesEmptyBatch)
        {
            ::DotProduct_HandlesEmptyBatch();
        }

        TEST_METHOD(RefAndSSE_AoS_GiveSameResult)
        {
            ::RefAndSSE_AoS_GiveSameResult(1);
        }

        TEST_METHOD(RefAndSSE_AoS_GiveSameResult)
        {
            ::RefAndSSE_AoS_GiveSameResult(4);
        }

        TEST_METHOD(RefAndSSE_AoS_GiveSameResult)
        {
            ::RefAndSSE_SoA_GiveSameResult(4);
        }


        TEST_METHOD(RefAndSSE_AoS_GiveSameResult)
        {
            ::RefAndSSE_AoS_GiveSameResult(7);
        }


        TEST_METHOD(RefAndSSE_SoA_GiveSameResult)
        {
            ::RefAndSSE_SoA_GiveSameResult(1000);
        }
    };
}