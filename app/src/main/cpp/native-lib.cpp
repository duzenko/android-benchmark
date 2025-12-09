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
std::string runBenchmark(const std::string& typeName, int num_threads, Worker worker) {
    long long num_elements = 1024;
    long long last_successful_num_elements = 0;
    std::chrono::duration<double, std::milli> last_duration;
    const long long max_allocation_bytes = 1024 * 1024 * 1024; // 1 GB limit
    const T test_repetitions = 10;

    while (true) {
        if ((num_elements * sizeof(T)) > max_allocation_bytes) {
            break;
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
            worker(data, 0, num_elements, test_repetitions);
        } else {
            std::vector<std::thread> threads;
            long long chunk_size = num_elements / num_threads;
            for (int i = 0; i < num_threads; ++i) {
                long long start_index = i * chunk_size;
                long long end_index = (i == num_threads - 1) ? num_elements : start_index + chunk_size;
                threads.emplace_back(worker, data, start_index, end_index, test_repetitions);
            }
            for (auto& t : threads) {
                t.join();
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        
        last_duration = end - start;
        __android_log_print(ANDROID_LOG_DEBUG, "BenchmarkDebug", "Test: %s, Elements: %lld, Reps: %d, Duration: %f ms", typeName.c_str(), num_elements, test_repetitions, last_duration.count());
        last_successful_num_elements = num_elements;
        
        delete[] data; // Free memory

        if (last_duration.count() > 100.0) {
            break;
        }

        num_elements *= 2;
    }

    if (last_successful_num_elements == 0) {
        std::stringstream result;
        result << typeName << "|0|0";
        return result.str();
    }

    long long size_in_bytes = last_successful_num_elements * sizeof(T);
    double duration_ms = last_duration.count();
    
    double total_bytes_processed = size_in_bytes * test_repetitions;
    double bandwidth = (total_bytes_processed / (1024.0 * 1024.0)) / (duration_ms / 1000.0);
    
    double performance_metric = (last_successful_num_elements * test_repetitions) / (duration_ms / 1000.0);

    std::stringstream result;
    result << typeName << "|" << performance_metric << "|" << bandwidth;

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

    auto indexed_worker = [](auto* data, long long start_index, long long end_index, auto repetitions) {
        for (auto j = 0; j < repetitions; ++j) {
            for (long long i = start_index; i < end_index; ++i) {
                data[i] = j;
            }
        }
    };

    std::string result8 = runBenchmark<uint8_t>("8-bit", 1, indexed_worker);
    jstring result8_jstr = env->NewStringUTF(result8.c_str());
    env->CallVoidMethod(callback, onProgressUpdate, result8_jstr);
    env->DeleteLocalRef(result8_jstr);

    std::string result16 = runBenchmark<uint16_t>("16-bit", 1, indexed_worker);
    jstring result16_jstr = env->NewStringUTF(result16.c_str());
    env->CallVoidMethod(callback, onProgressUpdate, result16_jstr);
    env->DeleteLocalRef(result16_jstr);

    std::string result32 = runBenchmark<uint32_t>("32-bit", 1, indexed_worker);
    jstring result32_jstr = env->NewStringUTF(result32.c_str());
    env->CallVoidMethod(callback, onProgressUpdate, result32_jstr);
    env->DeleteLocalRef(result32_jstr);

    std::string result64 = runBenchmark<uint64_t>("64-bit", 1, indexed_worker);
    jstring result64_jstr = env->NewStringUTF(result64.c_str());
    env->CallVoidMethod(callback, onProgressUpdate, result64_jstr);
    env->DeleteLocalRef(result64_jstr);

#ifdef __SIZEOF_INT128__
    using uint128_t = unsigned __int128;
    std::string result128 = runBenchmark<uint128_t>("128-bit", 1, indexed_worker);
    jstring result128_jstr = env->NewStringUTF(result128.c_str());
    env->CallVoidMethod(callback, onProgressUpdate, result128_jstr);
    env->DeleteLocalRef(result128_jstr);
#endif

    const unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores > 0) {
#ifdef __SIZEOF_INT128__
        using uint128_t = unsigned __int128;
        std::string test_name = "128-bit (" + std::to_string(num_cores) + " thr)";
        std::string result_all_cores = runBenchmark<uint128_t>(test_name, num_cores, indexed_worker);
        jstring result_all_cores_jstr = env->NewStringUTF(result_all_cores.c_str());
        env->CallVoidMethod(callback, onProgressUpdate, result_all_cores_jstr);
        env->DeleteLocalRef(result_all_cores_jstr);
#else
        std::string test_name = "64-bit (" + std::to_string(num_cores) + " thr)";
        std::string result_all_cores = runBenchmark<uint64_t>(test_name, num_cores, indexed_worker);
        jstring result_all_cores_jstr = env->NewStringUTF(result_all_cores.c_str());
        env->CallVoidMethod(callback, onProgressUpdate, result_all_cores_jstr);
        env->DeleteLocalRef(result_all_cores_jstr);
#endif

        auto memset_worker = [](uint8_t* data, long long start_index, long long end_index, int repetitions) {
             for (int j = 0; j < repetitions; ++j) {
                memset(data + start_index, j, (end_index - start_index) * sizeof(uint8_t));
            }
        };
        std::string memset_test_name = "memset (" + std::to_string(num_cores) + " thr)";
        std::string result_memset_threaded = runBenchmark<uint8_t>(memset_test_name, num_cores, memset_worker);
        jstring result_memset_threaded_jstr = env->NewStringUTF(result_memset_threaded.c_str());
        env->CallVoidMethod(callback, onProgressUpdate, result_memset_threaded_jstr);
        env->DeleteLocalRef(result_memset_threaded_jstr);
    }

    env->CallVoidMethod(callback, onFinished);
}