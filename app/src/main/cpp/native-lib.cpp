#include <jni.h>
#include <string>
#include <chrono>
#include <vector>
#include <sstream>
#include <cstdint>
#include <iomanip>
#include <new> // Required for std::bad_alloc
#include <android/log.h>
#include <thread>
#include <cstring> // For memset
#include <functional> // For std::function
#include <type_traits> // For std::remove_pointer_t

// Unified benchmark function
template<typename T, typename Worker>
std::string runBenchmark(int num_threads, Worker worker) {
    long long num_elements = 1024;
    long long last_successful_num_elements = 0;
    std::chrono::duration<double, std::milli> last_duration;
    const long long max_allocation_bytes = 1024 * 1024 * 1024; // 1 GB limit
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
        __android_log_print(ANDROID_LOG_DEBUG, "BenchmarkDebug", "Elements: %lld, Reps: %d, Duration: %f ms", num_elements, test_repetitions, last_duration.count());
        last_successful_num_elements = num_elements;
        
        delete[] data; // Free memory

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

extern "C" JNIEXPORT jint JNICALL
Java_name_duzenko_benchmark_BenchmarkModel_getTestCount(
        JNIEnv* env,
        jobject /* this */) {
    int count = 4; // 8, 16, 32, 64-bit single-threaded tests
#ifdef __SIZEOF_INT128__
    count++; // Add 128-bit single-threaded test
#endif
    const unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores > 0) {
        count++; // All-cores test
        count++; // All-cores memset test
    }
    return count;
}


extern "C" JNIEXPORT void JNICALL
Java_name_duzenko_benchmark_BenchmarkModel_runAllMemoryBenchmarks(
        JNIEnv* env,
        jobject /* this */,
        jobject callback) {

    jclass callbackClass = env->GetObjectClass(callback);
    jmethodID onProgressUpdate = env->GetMethodID(callbackClass, "onProgressUpdate", "(Ljava/lang/String;)V");
    jmethodID onFinished = env->GetMethodID(callbackClass, "onFinished", "()V");

    auto run_and_report = [&](const std::string& test_name, auto&& benchmark_call) {
        std::string raw_metrics = benchmark_call();
        std::string final_result = test_name + "|" + raw_metrics;
        jstring result_jstr = env->NewStringUTF(final_result.c_str());
        env->CallVoidMethod(callback, onProgressUpdate, result_jstr);
        env->DeleteLocalRef(result_jstr);
    };

    auto indexed_worker = [](auto* start, auto* end, auto repetitions) {
        for (auto j = 0; j < repetitions; ++j) {
            for (auto* p = start; p < end; ++p) {
                *p = (size_t)p;
            }
        }
    };

    run_and_report("8-bit", [&]{ return runBenchmark<uint8_t>(1, indexed_worker); });
    run_and_report("16-bit", [&]{ return runBenchmark<uint16_t>(1, indexed_worker); });
    run_and_report("32-bit", [&]{ return runBenchmark<uint32_t>(1, indexed_worker); });
    run_and_report("64-bit", [&]{ return runBenchmark<uint64_t>(1, indexed_worker); });

#ifdef __SIZEOF_INT128__
    using uint128_t = unsigned __int128;
    run_and_report("128-bit", [&]{ return runBenchmark<uint128_t>(1, indexed_worker); });
#endif

    const unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores > 0) {
#ifdef __SIZEOF_INT128__
        using uint128_t = unsigned __int128;
        std::string test_name = "128-bit (" + std::to_string(num_cores) + " thr)";
        run_and_report(test_name, [&]{ return runBenchmark<uint128_t>(num_cores, indexed_worker); });
#else
        std::string test_name = "64-bit (" + std::to_string(num_cores) + " thr)";
        run_and_report(test_name, [&]{ return runBenchmark<uint64_t>(num_cores, indexed_worker); });
#endif

        auto memset_worker = [](uint8_t* start, uint8_t* end, int repetitions) {
             for (int j = 0; j < repetitions; ++j) {
                memset(start, j, (end - start));
            }
        };
        std::string memset_test_name = "memset (" + std::to_string(num_cores) + " thr)";
        run_and_report(memset_test_name, [&]{ return runBenchmark<uint8_t>(num_cores, memset_worker); });
    }

    env->CallVoidMethod(callback, onFinished);
}