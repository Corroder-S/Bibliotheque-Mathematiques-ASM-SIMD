#include <gtest/gtest.h>
#include "Platform/CpuFeatures.h"

using Platform::CpuFeature;
using Platform::CpuFeatureSnapshot;
using Platform::CpuFeatures;

namespace
{
    CpuFeatureSnapshot FullSnapshot()
    {
        CpuFeatureSnapshot snapshot;
        snapshot.maxBasicLeaf = 7;
        snapshot.leaf1Edx = (1u << 25) | (1u << 26);
        snapshot.leaf1Ecx = (1u << 0) | (1u << 9) | (1u << 12) |
            (1u << 19) | (1u << 20) | (1u << 26) | (1u << 27) | (1u << 28);
        snapshot.leaf7Ebx = (1u << 5) | (1u << 16) | (1u << 17) | (1u << 30) | (1u << 31);
        snapshot.xcr0 = 0xE6;
        snapshot.osSse = true;
        snapshot.osSse2 = true;
        return snapshot;
    }
}

TEST(CpuFeaturesTests, EmptySnapshotSupportsNothing)
{
    const auto cpu = CpuFeatures::FromSnapshot({});
    EXPECT_FALSE(cpu.HasHardware(CpuFeature::SSE));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX512F));
}

TEST(CpuFeaturesTests, FullSnapshotEnablesEachExposedFeature)
{
    const auto cpu = CpuFeatures::FromSnapshot(FullSnapshot());
    for (const auto feature : {CpuFeature::SSE, CpuFeature::SSE2, CpuFeature::SSE3,
        CpuFeature::SSSE3, CpuFeature::SSE41, CpuFeature::SSE42, CpuFeature::AVX,
        CpuFeature::AVX2, CpuFeature::FMA, CpuFeature::AVX512F, CpuFeature::AVX512DQ,
        CpuFeature::AVX512BW, CpuFeature::AVX512VL})
    {
        EXPECT_TRUE(cpu.HasHardware(feature));
        EXPECT_TRUE(cpu.CanUse(feature));
    }
}

TEST(CpuFeaturesTests, EachLeaf1FeatureUsesItsOwnBit)
{
    struct Case { CpuFeature feature; unsigned int bit; bool edx; };
    const Case cases[] = {
        {CpuFeature::SSE,25,true}, {CpuFeature::SSE2,26,true},
        {CpuFeature::SSE3,0,false}, {CpuFeature::SSSE3,9,false},
        {CpuFeature::SSE41,19,false}, {CpuFeature::SSE42,20,false},
        {CpuFeature::FMA,12,false}, {CpuFeature::AVX,28,false}};
    for (const auto& item : cases)
    {
        CpuFeatureSnapshot snapshot;
        snapshot.maxBasicLeaf = 1;
        if (item.edx) { snapshot.leaf1Edx = 1u << item.bit; }
        else { snapshot.leaf1Ecx = 1u << item.bit; }
        const auto cpu = CpuFeatures::FromSnapshot(snapshot);
        for (const auto& other : cases)
            EXPECT_TRUE(cpu.HasHardware(other.feature) == (item.feature == other.feature));
    }
}

TEST(CpuFeaturesTests, EachLeaf7FeatureUsesItsOwnBit)
{
    struct Case { CpuFeature feature; unsigned int bit; };
    const Case cases[] = {{CpuFeature::AVX2,5}, {CpuFeature::AVX512F,16},
        {CpuFeature::AVX512DQ,17}, {CpuFeature::AVX512BW,30}, {CpuFeature::AVX512VL,31}};
    for (const auto& item : cases)
    {
        CpuFeatureSnapshot snapshot;
        snapshot.maxBasicLeaf = 7;
        snapshot.leaf7Ebx = 1u << item.bit;
        const auto cpu = CpuFeatures::FromSnapshot(snapshot);
        for (const auto& other : cases)
            EXPECT_TRUE(cpu.HasHardware(other.feature) == (item.feature == other.feature));
    }
}

TEST(CpuFeaturesTests, UnsupportedLeavesAreIgnored)
{
    auto snapshot = FullSnapshot();
    snapshot.maxBasicLeaf = 0;
    EXPECT_FALSE(CpuFeatures::FromSnapshot(snapshot).HasHardware(CpuFeature::SSE));
    snapshot.maxBasicLeaf = 1;
    EXPECT_TRUE(CpuFeatures::FromSnapshot(snapshot).CanUse(CpuFeature::AVX));
    EXPECT_FALSE(CpuFeatures::FromSnapshot(snapshot).HasHardware(CpuFeature::AVX2));
}

TEST(CpuFeaturesTests, SseRequiresOperatingSystemSupport)
{
    auto snapshot = FullSnapshot();
    snapshot.osSse = false;
    const auto cpu = CpuFeatures::FromSnapshot(snapshot);
    EXPECT_TRUE(cpu.HasHardware(CpuFeature::SSE));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::SSE));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::SSE42));
    snapshot.osSse = true;
    snapshot.osSse2 = false;
    EXPECT_FALSE(CpuFeatures::FromSnapshot(snapshot).CanUse(CpuFeature::SSE2));
}

TEST(CpuFeaturesTests, AvxRequiresHardwareXsaveAndOsxsave)
{
    for (unsigned int missing : {26u,27u,28u})
    {
        auto snapshot = FullSnapshot();
        snapshot.leaf1Ecx &= ~(1u << missing);
        const auto cpu = CpuFeatures::FromSnapshot(snapshot);
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX2));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::FMA));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX512F));
    }
}

TEST(CpuFeaturesTests, AvxRequiresBothXmmAndYmmState)
{
    for (std::uint64_t state : {0ull,2ull,4ull})
    {
        auto snapshot = FullSnapshot();
        snapshot.xcr0 = state;
        const auto cpu = CpuFeatures::FromSnapshot(snapshot);
        EXPECT_TRUE(cpu.HasHardware(CpuFeature::AVX2));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX2));
    }
}

TEST(CpuFeaturesTests, Avx2DoesNotImplyFma)
{
    auto snapshot = FullSnapshot();
    snapshot.leaf1Ecx &= ~(1u << 12);
    const auto cpu = CpuFeatures::FromSnapshot(snapshot);
    EXPECT_TRUE(cpu.CanUse(CpuFeature::AVX2));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::FMA));
}

TEST(CpuFeaturesTests, Avx512RequiresEveryExtendedStateBit)
{
    for (unsigned int missing : {1u,2u,5u,6u,7u})
    {
        auto snapshot = FullSnapshot();
        snapshot.xcr0 &= ~(std::uint64_t{1} << missing);
        const auto cpu = CpuFeatures::FromSnapshot(snapshot);
        EXPECT_TRUE(cpu.HasHardware(CpuFeature::AVX512F));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX512F));
        EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX512BW));
    }
}

TEST(CpuFeaturesTests, Avx512SubsetsRequireFoundation)
{
    auto snapshot = FullSnapshot();
    snapshot.leaf7Ebx &= ~(1u << 16);
    const auto cpu = CpuFeatures::FromSnapshot(snapshot);
    EXPECT_TRUE(cpu.HasHardware(CpuFeature::AVX512VL));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX512VL));
    EXPECT_FALSE(cpu.CanUse(CpuFeature::AVX512DQ));
}

TEST(CpuFeaturesTests, UnknownFeatureIsRejected)
{
    const auto cpu = CpuFeatures::FromSnapshot(FullSnapshot());
    const auto unknown = static_cast<CpuFeature>(999);
    EXPECT_FALSE(cpu.HasHardware(unknown));
    EXPECT_FALSE(cpu.CanUse(unknown));
}
