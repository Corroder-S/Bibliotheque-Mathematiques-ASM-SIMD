#include "Maths/ASM/ASM_Vec3.h"
#include "TestHelpers.h"
#include <gtest/gtest.h>
#include <type_traits>
#include <numbers>


namespace
{
    void ASM_Vec3_DotProduct()
    {

        using V = Maths::Vec3<float>;
        const V a = V{1, 2, 3};
        const V b = V{2, 3, 4};
        TestHelpers::Near(float{20}, a.Dot(b));
    }
}

TEST(ASM_Vec3Tests, ASM_Vec3_DotProduct) { ASM_Vec3_DotProduct(); }

