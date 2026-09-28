#pragma once

#include <cstdint>
#include <string>

namespace Platform
{
    enum class CpuFeature
    {
        SSE, SSE2, SSE3, SSSE3, SSE41, SSE42,
        AVX, AVX2, FMA, AVX512F, AVX512DQ, AVX512BW, AVX512VL
    };

    // Small, synthetic input for deterministic feature-decoding tests.
    // Not needed by application code: use CpuFeatures::Detect().
    struct CpuFeatureSnapshot
    {
        std::uint32_t maxBasicLeaf = 0;
        std::uint32_t leaf1Ecx = 0;
        std::uint32_t leaf1Edx = 0;
        std::uint32_t leaf7Ebx = 0;
        std::uint64_t xcr0 = 0;
        bool osSse = false;
        bool osSse2 = false;
    };

    class CpuFeatures
    {
    public:
        // MSVC, Windows x86/x64. Queries only the required leaves.
        static CpuFeatures Detect();
        static CpuFeatures FromSnapshot(const CpuFeatureSnapshot& snapshot);

        const std::string& Vendor() const noexcept;
        const std::string& Brand() const noexcept;
        bool HasHardware(CpuFeature feature) const noexcept;
        bool CanUse(CpuFeature feature) const noexcept;

    private:
        CpuFeatureSnapshot snapshot_{};
        std::string vendor_;
        std::string brand_;
    };
}
