#include <iostream>
#include <iomanip>
#include <string>
#include <cstdint>
#include <thread>

// Enable debug logging for console
#define LOG_DEBUG(fmt, ...) // Disable verbose logging

#include "benchmark.h"

void printResult(const std::string& test_name, const std::string& raw_metrics) {
    BenchmarkResult result = parseBenchmarkResult(raw_metrics);

    std::cout << std::left << std::setw(20) << test_name << " | ";
    std::cout << std::right << std::setw(12) << result.num_elements << " elements | ";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(8) << result.duration_ms << " ms | ";
    std::cout << std::setw(8) << result.getBandwidthGBps() << " GB/s" << std::endl;
}

int main() {
    std::cout << "=== Memory Bandwidth Benchmark ===" << std::endl;
    std::cout << std::endl;

    const unsigned int num_cores = std::thread::hardware_concurrency();
    std::cout << "Detected CPU cores: " << num_cores << std::endl;
    std::cout << std::endl;

    std::cout << std::left << std::setw(20) << "Test Name" << " | ";
    std::cout << std::right << std::setw(12) << "Elements" << "          | ";
    std::cout << std::setw(8) << "Time" << "    | ";
    std::cout << std::setw(8) << "Bandwidth" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    auto indexed_worker = [](auto* start, auto* end, auto repetitions) {
        for (auto j = 0; j < repetitions; ++j) {
            for (auto* p = start; p < end; ++p) {
                *p = (size_t)p;
            }
        }
    };

    // Single-threaded tests
    printResult("8-bit", runBenchmark<uint8_t>(1, indexed_worker));
    printResult("16-bit", runBenchmark<uint16_t>(1, indexed_worker));
    printResult("32-bit", runBenchmark<uint32_t>(1, indexed_worker));
    printResult("64-bit", runBenchmark<uint64_t>(1, indexed_worker));

#ifdef __SIZEOF_INT128__
    using uint128_t = unsigned __int128;
    printResult("128-bit", runBenchmark<uint128_t>(1, indexed_worker));
#endif

    // Multi-threaded tests
    if (num_cores > 0) {
        std::cout << std::endl;
        std::cout << "Multi-threaded tests (" << num_cores << " threads):" << std::endl;
        std::cout << std::string(70, '-') << std::endl;

#ifdef __SIZEOF_INT128__
        using uint128_t = unsigned __int128;
        std::string test_name = "128-bit (" + std::to_string(num_cores) + " thr)";
        printResult(test_name, runBenchmark<uint128_t>(num_cores, indexed_worker));
#else
        std::string test_name = "64-bit (" + std::to_string(num_cores) + " thr)";
        printResult(test_name, runBenchmark<uint64_t>(num_cores, indexed_worker));
#endif

        auto memset_worker = [](uint8_t* start, uint8_t* end, int repetitions) {
            for (int j = 0; j < repetitions; ++j) {
                memset(start, j, (end - start));
            }
        };
        std::string memset_test_name = "memset (" + std::to_string(num_cores) + " thr)";
        printResult(memset_test_name, runBenchmark<uint8_t>(num_cores, memset_worker));
    }

    std::cout << std::endl;
    std::cout << "Benchmark complete." << std::endl;
    std::cin.get();

    return 0;
}
