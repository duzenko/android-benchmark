#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <string>
#include <vector>

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

namespace cpu_features {

class X86Features {
public:
    bool has_sse2 = false;
    bool has_avx = false;
    bool has_avx2 = false;
    bool has_avx512f = false;

    static const X86Features& get() {
        static const X86Features features = detect();
        return features;
    }

private:
    static X86Features detect() {
        X86Features f;

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        std::vector<int> cpuInfo(4);

#if defined(_MSC_VER)
        __cpuid(cpuInfo.data(), 1);
#else
        __asm__ __volatile__(
            "cpuid":
            "=a" (cpuInfo[0]), "=b" (cpuInfo[1]), "=c" (cpuInfo[2]), "=d" (cpuInfo[3]):
            "a" (1), "c" (0)
        );
#endif

        f.has_sse2 = (cpuInfo[3] & (1 << 26)) != 0;
        f.has_avx = (cpuInfo[2] & (1 << 28)) != 0;

        // To check for AVX2 and AVX512F, we need to check leaf 7
#if defined(_MSC_VER)
         __cpuidex(cpuInfo.data(), 7, 0);
#else
        __asm__ __volatile__(
            "cpuid":
            "=a" (cpuInfo[0]), "=b" (cpuInfo[1]), "=c" (cpuInfo[2]), "=d" (cpuInfo[3]):
            "a" (7), "c" (0)
        );
#endif

        f.has_avx2 = (cpuInfo[1] & (1 << 5)) != 0;
        f.has_avx512f = (cpuInfo[1] & (1 << 16)) != 0;

#endif // x86 check
        return f;
    }
};

} // namespace cpu_features

#endif // CPU_FEATURES_H
