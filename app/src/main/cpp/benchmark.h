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

// Optional logging macro - define your own LOG_DEBUG before including this header
#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt, ...)
#endif

// Unified benchmark function
template<typename T, typename Worker>
std::string runBenchmark(int num_threads, Worker worker) {
    long long num_elements = 1024;
    long long last_successful_num_elements = 0;
    std::chrono::duration<double, std::milli> last_duration;
    const long long max_allocation_bytes = 1024LL * 1024 * 1024; // 1 GB limit
    int test_repetitions = 10;

    while (true) {
        if ((num_elements * sizeof(T)) > max_allocation_bytes) {
            test_repetitions *= 2;
        }

        T* data = nullptr;
        try {
            data = new T[num_elements];
        } catch (const std::bad_alloc& e) {
            break;
        }

        // Warm-up run
        memset(data, 0, num_elements * sizeof(T));

        auto start = std::chrono::high_resolution_clock::now();

        // Timed run
        if (num_threads == 1) {
            worker(data, data + num_elements, (T)test_repetitions);
        } else {
            std::vector<std::thread> threads;
            long long chunk_size = num_elements / num_threads;
            for (int i = 0; i < num_threads; ++i) {
                long long start_index = i * chunk_size;
                long long end_index = (i == num_threads - 1) ? num_elements : start_index + chunk_size;
                threads.emplace_back(worker, data + start_index, data + end_index, test_repetitions);
            }
            for (auto& t : threads) {
                t.join();
            }
        }

        auto end = std::chrono::high_resolution_clock::now();

        last_duration = end - start;
        LOG_DEBUG("Elements: %lld, Reps: %d, Duration: %f ms", num_elements, test_repetitions, last_duration.count());
        last_successful_num_elements = num_elements;

        delete[] data;

        if (last_duration.count() > 100.0) {
            break;
        }

        num_elements *= 2;
    }

    if (last_successful_num_elements == 0) {
        std::stringstream result;
        result << "0|" << sizeof(T) << "|0|0";
        return result.str();
    }

    std::stringstream result;
    result << last_successful_num_elements << "|" << sizeof(T) << "|" << last_duration.count() << "|" << test_repetitions;
    return result.str();
}

// Parse benchmark result string
struct BenchmarkResult {
    long long num_elements;
    size_t element_size;
    double duration_ms;
    int repetitions;

    double getBandwidthGBps() const {
        if (duration_ms <= 0) return 0;
        double total_bytes = (double)num_elements * element_size * repetitions;
        double seconds = duration_ms / 1000.0;
        return (total_bytes / (1024.0 * 1024.0 * 1024.0)) / seconds;
    }
};

inline BenchmarkResult parseBenchmarkResult(const std::string& raw) {
    BenchmarkResult result = {0, 0, 0, 0};
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

#endif // BENCHMARK_H
