#include "TestHelpers.h"
#include "Maths/Vec3.h"
#include "Maths/VecBatch.h"
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace TestHelpers;

namespace Test::Ref
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

    void DotProduct_ReferenceMatchesKnownValues()
    {
        std::vector <Maths::Vec3f > a = { Maths::Vec3f{1,2,3}, Maths::Vec3f{2,3,4} };
        std::vector<Maths::Vec3f> b = { Maths::Vec3f{2,3,4}, Maths::Vec3f{1,0,0} };
        std::vector<float> out(2);

        Maths::Ref::DotProduct_AOS(a.data(), b.data(), out.data(), 2);

        Near(20.0f, out[0]);
        Near(2.0f, out[1]);
    }

    void DotProduct_HandlesEmptyBatch()
    {
        std::vector<Maths::Vec3f> a, b;
        std::vector<float> out;

        Maths::Ref::DotProduct_AOS(a.data(), b.data(), out.data(), 0);
        Maths::SSE::DotProduct_AOS(a.data(), b.data(), out.data(), 0);
    }

    void DotProduct_SSE_AoS_MatchesReferenceForSize(size_t count)
    {
        std::vector<Maths::Vec3f> a, b;
        BuildKnownData(a, b, count);

        std::vector<float> outRef(count);
        std::vector<float> outSse(count);

        Maths::Ref::DotProduct_AOS(a.data(), b.data(), outRef.data(), count);
        Maths::SSE::DotProduct_AOS(a.data(), b.data(), outSse.data(), count);

        for (size_t i = 0; i < count; ++i)
        {
            Near(outRef[i], outSse[i]);
        }
    }

    void DotProduct_SSE_SoA_MatchesReferenceForSize(size_t count)
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
            Near(outRef[i], outSoa[i]);
        }
    }
}