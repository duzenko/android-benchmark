#include <iostream>
#include <iomanip>
#include <string>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <cstring>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define LOG_DEBUG(fmt, ...)

#include "benchmark.h"

namespace {

constexpr int kTestNameWidth = 28;
constexpr int kSeparatorWidth = 78;

void printHeader() {
    std::cout << std::left << std::setw(kTestNameWidth) << "Test Name" << " | ";
    std::cout << std::right << std::setw(12) << "Elements" << "          | ";
    std::cout << std::setw(8) << "Time" << "    | ";
    std::cout << std::setw(8) << "Bandwidth" << std::endl;
    std::cout << std::string(kSeparatorWidth, '-') << std::endl;
}

void printResult(const std::string& test_name, const std::string& raw_metrics) {
    BenchmarkResult result = parseBenchmarkResult(raw_metrics);

    std::cout << std::left << std::setw(kTestNameWidth) << test_name << " | ";
    std::cout << std::right << std::setw(12) << result.num_elements << " elements | ";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(8) << result.duration_ms << " ms | ";
    std::cout << std::setw(8) << result.getBandwidthGBps() << " GB/s" << std::endl;
}

void printSection(const std::string& name) {
    std::cout << std::endl << name << ":" << std::endl;
    std::cout << std::string(kSeparatorWidth, '-') << std::endl;
}

template<typename T, typename Worker>
void runScalingTest(const std::string& base_name, Worker worker, unsigned int max_cores) {
    double prev_bandwidth = 0;

    for (unsigned int threads = 2; threads <= max_cores; ) {
        std::string name = base_name + " (" + std::to_string(threads) + " thr)";
        std::string raw = runBenchmark<T>(threads, worker);
        BenchmarkResult result = parseBenchmarkResult(raw);
        double bandwidth = result.getBandwidthGBps();

        printResult(name, raw);

        if (prev_bandwidth > 0) {
            double improvement = (bandwidth - prev_bandwidth) / prev_bandwidth;
            if (improvement < benchmark::kMinImprovementThreshold) {
                std::cout << "  (scaling stopped - <5% improvement)" << std::endl;
                break;
            }
        }

        prev_bandwidth = bandwidth;

        if (max_cores > 16) {
            threads *= 2;
        } else {
            threads += 2;
        }
    }
}

#ifndef _WIN32
char getKeyUnix() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

char getKey() {
#ifdef _WIN32
    return static_cast<char>(_getch());
#else
    return getKeyUnix();
#endif
}

void runBenchmarks() {
    const unsigned int num_cores = std::thread::hardware_concurrency();
    auto indexed_worker = benchmark::makeIndexedWorker();
    auto memset_worker = benchmark::makeMemsetWorker();

    std::cout << std::endl;
    std::cout << "Detected CPU cores: " << num_cores << std::endl;
    std::cout << std::endl;

    printHeader();

    // Single-threaded tests
    std::cout << "Single-threaded:" << std::endl;
    printResult("64-bit", runBenchmark<uint64_t>(1, indexed_worker));

#if defined(HAS_SSE2) || defined(HAS_AVX_RUNTIME_CHECK)
    printResult("128-bit SSE", runBenchmark<__m128i>(1, sse_worker));
    printResult("128-bit SSE NT", runBenchmark<__m128i>(1, sse_stream_worker));
#endif

#if defined(HAS_AVX)
    printResult("256-bit AVX", runBenchmark<__m256i>(1, avx_worker));
    printResult("256-bit AVX NT", runBenchmark<__m256i>(1, avx_stream_worker));
#elif defined(HAS_AVX_RUNTIME_CHECK)
    if (hasAVX()) {
        printResult("256-bit AVX", runBenchmark<__m256i>(1, avx_worker));
        printResult("256-bit AVX NT", runBenchmark<__m256i>(1, avx_stream_worker));
    }
#endif

#if defined(HAS_AVX512)
    printResult("512-bit AVX-512", runBenchmark<__m512i>(1, avx512_worker));
    printResult("512-bit AVX-512 NT", runBenchmark<__m512i>(1, avx512_stream_worker));
#elif defined(HAS_AVX_RUNTIME_CHECK)
    if (hasAVX512()) {
        printResult("512-bit AVX-512", runBenchmark<__m512i>(1, avx512_worker));
        printResult("512-bit AVX-512 NT", runBenchmark<__m512i>(1, avx512_stream_worker));
    }
#endif

#ifdef HAS_NEON
    printResult("128-bit NEON", runBenchmark<uint8x16_t>(1, neon_worker));
    printResult("128-bit NEON NT", runBenchmark<uint8x16_t>(1, neon_stream_worker));
#endif

    printResult("memset", runBenchmark<uint8_t>(1, memset_worker));

    // Multi-threaded scaling tests
    if (num_cores >= 2) {
        printSection("Multi-threaded scaling (stops at <5% improvement)");

        std::cout << std::endl << "64-bit:" << std::endl;
        runScalingTest<uint64_t>("64-bit", indexed_worker, num_cores);

#if defined(HAS_AVX)
        std::cout << std::endl << "256-bit AVX:" << std::endl;
        runScalingTest<__m256i>("256-bit AVX", avx_worker, num_cores);

        std::cout << std::endl << "256-bit AVX NT:" << std::endl;
        runScalingTest<__m256i>("256-bit AVX NT", avx_stream_worker, num_cores);
#elif defined(HAS_AVX_RUNTIME_CHECK)
        if (hasAVX()) {
            std::cout << std::endl << "256-bit AVX:" << std::endl;
            runScalingTest<__m256i>("256-bit AVX", avx_worker, num_cores);

            std::cout << std::endl << "256-bit AVX NT:" << std::endl;
            runScalingTest<__m256i>("256-bit AVX NT", avx_stream_worker, num_cores);
        }
#endif

#if defined(HAS_AVX512)
        std::cout << std::endl << "512-bit AVX-512 NT:" << std::endl;
        runScalingTest<__m512i>("512-bit AVX-512 NT", avx512_stream_worker, num_cores);
#elif defined(HAS_AVX_RUNTIME_CHECK)
        if (hasAVX512()) {
            std::cout << std::endl << "512-bit AVX-512 NT:" << std::endl;
            runScalingTest<__m512i>("512-bit AVX-512 NT", avx512_stream_worker, num_cores);
        }
#endif

#ifdef HAS_NEON
        std::cout << std::endl << "128-bit NEON:" << std::endl;
        runScalingTest<uint8x16_t>("128-bit NEON", neon_worker, num_cores);

        std::cout << std::endl << "128-bit NEON NT:" << std::endl;
        runScalingTest<uint8x16_t>("128-bit NEON NT", neon_stream_worker, num_cores);
#endif

        std::cout << std::endl << "memset:" << std::endl;
        runScalingTest<uint8_t>("memset", memset_worker, num_cores);
    }

    std::cout << std::endl;
    std::cout << "Benchmark complete." << std::endl;
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [--once]" << std::endl;
    std::cout << "  --once    Run benchmarks once and exit" << std::endl;
    std::cout << "  (default) Interactive mode: press 't' to test, 'q' to quit" << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
    bool run_once = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--once") == 0 || strcmp(argv[i], "-o") == 0) {
            run_once = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
    }

    std::cout << "=== Memory Bandwidth Benchmark ===" << std::endl;

    if (run_once) {
        runBenchmarks();
        return 0;
    }

    std::cout << std::endl;
    std::cout << "Press 't' to run test, 'q' to quit" << std::endl;

    while (true) {
        char key = getKey();

        if (key == 't' || key == 'T') {
            runBenchmarks();
            std::cout << std::endl;
            std::cout << "Press 't' to run test again, 'q' to quit" << std::endl;
        } else if (key == 'q' || key == 'Q') {
            std::cout << "Exiting..." << std::endl;
            break;
        }
    }

    return 0;
}
