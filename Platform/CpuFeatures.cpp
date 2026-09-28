#include "CpuFeatures.h"

#include <bit>
#include <cstring>
#include <stdexcept>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#include <immintrin.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace
{
    bool Bit(const std::uint32_t value, const unsigned int index) noexcept
    {
        return (value & (std::uint32_t{1} << index)) != 0;
    }
}

namespace Platform
{
    CpuFeatures CpuFeatures::FromSnapshot(const CpuFeatureSnapshot& snapshot)
    {
        CpuFeatures result;
        result.snapshot_ = snapshot;
        return result;
    }

    CpuFeatures CpuFeatures::Detect()
    {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        static_assert(sizeof(int) == 4);
        CpuFeatures result;
        int registers[4]{};
        __cpuidex(registers, 0, 0);
        result.snapshot_.maxBasicLeaf = static_cast<std::uint32_t>(registers[0]);

        char vendor[13]{};
        std::memcpy(vendor, &registers[1], 4);     // EBX
        std::memcpy(vendor + 4, &registers[3], 4); // EDX
        std::memcpy(vendor + 8, &registers[2], 4); // ECX
        result.vendor_ = vendor;

        if (result.snapshot_.maxBasicLeaf >= 1)
        {
            __cpuidex(registers, 1, 0);
            result.snapshot_.leaf1Ecx = static_cast<std::uint32_t>(registers[2]);
            result.snapshot_.leaf1Edx = static_cast<std::uint32_t>(registers[3]);

            const bool xsave = Bit(result.snapshot_.leaf1Ecx, 26);
            const bool osxsave = Bit(result.snapshot_.leaf1Ecx, 27);
            if (xsave && osxsave)
            {
                // Never execute XGETBV before checking OSXSAVE.
                result.snapshot_.xcr0 = _xgetbv(0);
            }
        }
        if (result.snapshot_.maxBasicLeaf >= 7)
        {
            __cpuidex(registers, 7, 0);
            result.snapshot_.leaf7Ebx = static_cast<std::uint32_t>(registers[1]);
        }

        result.snapshot_.osSse = IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0;
        result.snapshot_.osSse2 = IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) != 0;

        __cpuidex(registers, std::bit_cast<int>(std::uint32_t{0x80000000}), 0);
        const auto maxExtendedLeaf = static_cast<std::uint32_t>(registers[0]);
        if (maxExtendedLeaf >= 0x80000004u)
        {
            char brand[49]{};
            for (std::uint32_t i = 0; i < 3; ++i)
            {
                __cpuidex(registers, std::bit_cast<int>(0x80000002u + i), 0);
                std::memcpy(brand + i * 16, registers, 16);
            }
            result.brand_ = brand;
            const auto first = result.brand_.find_first_not_of(' ');
            if (first == std::string::npos)
            {
                result.brand_.clear();
            }
            else
            {
                const auto last = result.brand_.find_last_not_of(' ');
                result.brand_ = result.brand_.substr(first, last - first + 1);
            }
        }
        return result;
#else
        throw std::runtime_error("CPU detection requires MSVC on Windows x86/x64");
#endif
    }

    const std::string& CpuFeatures::Vendor() const noexcept
    {
        return vendor_;
    }

    const std::string& CpuFeatures::Brand() const noexcept
    {
        return brand_;
    }

    bool CpuFeatures::HasHardware(const CpuFeature feature) const noexcept
    {
        if (snapshot_.maxBasicLeaf < 1)
        {
            return false;
        }
        switch (feature)
        {
        case CpuFeature::SSE: return Bit(snapshot_.leaf1Edx, 25);
        case CpuFeature::SSE2: return Bit(snapshot_.leaf1Edx, 26);
        case CpuFeature::SSE3: return Bit(snapshot_.leaf1Ecx, 0);
        case CpuFeature::SSSE3: return Bit(snapshot_.leaf1Ecx, 9);
        case CpuFeature::SSE41: return Bit(snapshot_.leaf1Ecx, 19);
        case CpuFeature::SSE42: return Bit(snapshot_.leaf1Ecx, 20);
        case CpuFeature::AVX: return Bit(snapshot_.leaf1Ecx, 28);
        case CpuFeature::FMA: return Bit(snapshot_.leaf1Ecx, 12);
        default: break;
        }
        if (snapshot_.maxBasicLeaf < 7)
        {
            return false;
        }
        switch (feature)
        {
        case CpuFeature::AVX2: return Bit(snapshot_.leaf7Ebx, 5);
        case CpuFeature::AVX512F: return Bit(snapshot_.leaf7Ebx, 16);
        case CpuFeature::AVX512DQ: return Bit(snapshot_.leaf7Ebx, 17);
        case CpuFeature::AVX512BW: return Bit(snapshot_.leaf7Ebx, 30);
        case CpuFeature::AVX512VL: return Bit(snapshot_.leaf7Ebx, 31);
        default: return false;
        }
    }

    bool CpuFeatures::CanUse(const CpuFeature feature) const noexcept
    {
        if (!HasHardware(feature))
        {
            return false;
        }
        if (feature == CpuFeature::SSE)
        {
            return snapshot_.osSse;
        }
        switch (feature)
        {
        case CpuFeature::SSE2:
        case CpuFeature::SSE3:
        case CpuFeature::SSSE3:
        case CpuFeature::SSE41:
        case CpuFeature::SSE42:
            return snapshot_.osSse && snapshot_.osSse2 && HasHardware(CpuFeature::SSE2);
        default: break;
        }

        const bool avxState = HasHardware(CpuFeature::AVX) &&
            Bit(snapshot_.leaf1Ecx, 26) && Bit(snapshot_.leaf1Ecx, 27) &&
            (snapshot_.xcr0 & 0x6u) == 0x6u; // XMM and YMM state.
        if (!avxState)
        {
            return false;
        }
        switch (feature)
        {
        case CpuFeature::AVX:
        case CpuFeature::AVX2:
        case CpuFeature::FMA:
            return true;
        case CpuFeature::AVX512F:
        case CpuFeature::AVX512DQ:
        case CpuFeature::AVX512BW:
        case CpuFeature::AVX512VL:
            return HasHardware(CpuFeature::AVX512F) &&
                (snapshot_.xcr0 & 0xE6u) == 0xE6u; // XMM, YMM, opmask, ZMM.
        default: return false;
        }
    }
}
