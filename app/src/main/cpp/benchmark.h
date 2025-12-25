#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <string>
#include <chrono>
#include <vector>
#include <sstream>
#include <cstdint>
#include <thread>
#include <cstring>
#include <functional>
#include <memory>

// ============================================================================
// Configuration Constants
// ============================================================================

namespace benchmark {

constexpr long long kInitialElements = 1024;
constexpr long long kMaxAllocationBytes = 1024LL * 1024 * 1024;  // 1 GB
constexpr double kTargetDurationMs = 100.0;
constexpr int kInitialRepetitions = 10;
constexpr double kMinImprovementThreshold = 0.05;  // 5%

}  // namespace benchmark

// ============================================================================
// SIMD Support Detection
// ============================================================================

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #define HAS_SSE2 1
    #include <emmintrin.h>
#endif

#if defined(__AVX__) || defined(__AVX2__)
    #define HAS_AVX 1
    #include <immintrin.h>
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    #include <intrin.h>
    #include <immintrin.h>
    #define HAS_AVX_RUNTIME_CHECK 1
#endif

#if defined(__AVX512F__)
    #define HAS_AVX512 1
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define HAS_NEON 1
    #include <arm_neon.h>
#endif

// ============================================================================
// Runtime CPU Feature Detection (MSVC)
// ============================================================================

#ifdef HAS_AVX_RUNTIME_CHECK
inline bool hasAVX() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    return (cpuInfo[2] & (1 << 28)) != 0;
}

inline bool hasAVX512() {
    int cpuInfo[4];
    __cpuidex(cpuInfo, 7, 0);
    return (cpuInfo[1] & (1 << 16)) != 0;
}
#endif

// ============================================================================
// Logging
// ============================================================================

#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt, ...)
#endif

// ============================================================================
// Aligned Memory Allocation
// ============================================================================

namespace benchmark {

template<typename T>
T* allocateAligned(size_t count, size_t alignment = 64) {
#ifdef _MSC_VER
    return static_cast<T*>(_aligned_malloc(count * sizeof(T), alignment));
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, count * sizeof(T)) != 0) {
        return nullptr;
    }
    return static_cast<T*>(ptr);
#endif
}

template<typename T>
void freeAligned(T* ptr) {
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

template<typename T>
struct AlignedDeleter {
    void operator()(T* ptr) const {
        freeAligned(ptr);
    }
};

template<typename T>
using AlignedPtr = std::unique_ptr<T[], AlignedDeleter<T>>;

template<typename T>
AlignedPtr<T> makeAligned(size_t count, size_t alignment = 64) {
    return AlignedPtr<T>(allocateAligned<T>(count, alignment));
}

}  // namespace benchmark

// ============================================================================
// Benchmark Result
// ============================================================================

struct BenchmarkResult {
    long long num_elements = 0;
    size_t element_size = 0;
    double duration_ms = 0.0;
    int repetitions = 0;

    double getBandwidthGBps() const {
        if (duration_ms <= 0) return 0;
        double total_bytes = static_cast<double>(num_elements) * element_size * repetitions;
        double seconds = duration_ms / 1000.0;
        return (total_bytes / (1024.0 * 1024.0 * 1024.0)) / seconds;
    }
};

inline BenchmarkResult parseBenchmarkResult(const std::string& raw) {
    BenchmarkResult result;
    std::stringstream ss(raw);
    std::string token;
    int idx = 0;
    while (std::getline(ss, token, '|')) {
        switch (idx++) {
            case 0: result.num_elements = std::stoll(token); break;
            case 1: result.element_size = std::stoull(token); break;
            case 2: result.duration_ms = std::stod(token); break;
            case 3: result.repetitions = std::stoi(token); break;
        }
    }
    return result;
}

// ============================================================================
// Core Benchmark Function
// ============================================================================

template<typename T, typename Worker>
std::string runBenchmark(int num_threads, Worker worker) {
    using namespace benchmark;

    long long num_elements = kInitialElements;
    long long last_successful_num_elements = 0;
    std::chrono::duration<double, std::milli> last_duration{};
    int test_repetitions = kInitialRepetitions;

    while (true) {
        if ((num_elements * sizeof(T)) > kMaxAllocationBytes) {
            test_repetitions *= 2;
        }

        auto data = makeAligned<T>(num_elements);
        if (!data) {
            break;
        }

        // Warm-up
        memset(data.get(), 0, num_elements * sizeof(T));

        auto start = std::chrono::high_resolution_clock::now();

        if (num_threads == 1) {
            worker(data.get(), data.get() + num_elements, test_repetitions);
        } else {
            std::vector<std::thread> threads;
            threads.reserve(num_threads);
            long long chunk_size = num_elements / num_threads;

            for (int i = 0; i < num_threads; ++i) {
                long long start_idx = i * chunk_size;
                long long end_idx = (i == num_threads - 1) ? num_elements : start_idx + chunk_size;
                threads.emplace_back(worker, data.get() + start_idx, data.get() + end_idx, test_repetitions);
            }

            for (auto& t : threads) {
                t.join();
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        last_duration = end - start;

        LOG_DEBUG("Elements: %lld, Reps: %d, Duration: %f ms",
                  num_elements, test_repetitions, last_duration.count());

        last_successful_num_elements = num_elements;

        if (last_duration.count() > kTargetDurationMs) {
            break;
        }

        num_elements *= 2;
    }

    std::stringstream result;
    if (last_successful_num_elements == 0) {
        result << "0|" << sizeof(T) << "|0|0";
    } else {
        result << last_successful_num_elements << "|"
               << sizeof(T) << "|"
               << last_duration.count() << "|"
               << test_repetitions;
    }
    return result.str();
}

// ============================================================================
// Common Worker Functions
// ============================================================================

namespace benchmark {

inline auto makeIndexedWorker() {
    return [](auto* start, auto* end, int repetitions) {
        using T = std::remove_pointer_t<decltype(start)>;
        for (int j = 0; j < repetitions; ++j) {
            for (auto* p = start; p < end; ++p) {
                *p = static_cast<T>(reinterpret_cast<size_t>(p));
            }
        }
    };
}

inline auto makeMemsetWorker() {
    return [](uint8_t* start, uint8_t* end, int repetitions) {
        for (int j = 0; j < repetitions; ++j) {
            memset(start, j, end - start);
        }
    };
}

}  // namespace benchmark

// ============================================================================
// x86 SIMD Workers (SSE/AVX/AVX-512)
// ============================================================================

#if defined(HAS_SSE2) || defined(HAS_AVX_RUNTIME_CHECK)
inline void sse_worker(__m128i* start, __m128i* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        __m128i mm = _mm_set1_epi64x(static_cast<int64_t>(j));
        for (__m128i* p = start; p < end; ++p) {
            _mm_store_si128(p, mm);
        }
    }
}

inline void sse_stream_worker(__m128i* start, __m128i* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        __m128i mm = _mm_set1_epi64x(static_cast<int64_t>(j));
        for (__m128i* p = start; p < end; ++p) {
            _mm_stream_si128(p, mm);
        }
    }
    _mm_sfence();
}
#endif

#if defined(HAS_AVX) || defined(HAS_AVX_RUNTIME_CHECK)
inline void avx_worker(__m256i* start, __m256i* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        __m256i mm = _mm256_set1_epi64x(static_cast<int64_t>(j));
        for (__m256i* p = start; p < end; ++p) {
            _mm256_store_si256(p, mm);
        }
    }
}

inline void avx_stream_worker(__m256i* start, __m256i* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        __m256i mm = _mm256_set1_epi64x(static_cast<int64_t>(j));
        for (__m256i* p = start; p < end; ++p) {
            _mm256_stream_si256(p, mm);
        }
    }
    _mm_sfence();
}
#endif

#if defined(HAS_AVX512) || defined(HAS_AVX_RUNTIME_CHECK)
inline void avx512_worker(__m512i* start, __m512i* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        __m512i mm = _mm512_set1_epi64(static_cast<int64_t>(j));
        for (__m512i* p = start; p < end; ++p) {
            _mm512_store_si512(p, mm);
        }
    }
}

inline void avx512_stream_worker(__m512i* start, __m512i* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        __m512i mm = _mm512_set1_epi64(static_cast<int64_t>(j));
        for (__m512i* p = start; p < end; ++p) {
            _mm512_stream_si512(p, mm);
        }
    }
    _mm_sfence();
}
#endif

// ============================================================================
// ARM NEON Workers
// ============================================================================

#ifdef HAS_NEON
inline void neon_worker(uint8x16_t* start, uint8x16_t* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        uint8x16_t val = vdupq_n_u8(static_cast<uint8_t>(j));
        for (uint8x16_t* p = start; p < end; ++p) {
            vst1q_u8(reinterpret_cast<uint8_t*>(p), val);
        }
    }
}

inline void neon_stream_worker(uint8x16_t* start, uint8x16_t* end, int repetitions) {
    for (int j = 0; j < repetitions; ++j) {
        uint8x16_t val = vdupq_n_u8(static_cast<uint8_t>(j));
        for (uint8x16_t* p = start; p < end; ++p) {
            __builtin_nontemporal_store(val, p);
        }
    }
    __asm__ volatile("dmb ishst" ::: "memory");
}
#endif

#endif // BENCHMARK_H
