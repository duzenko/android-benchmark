#include <jni.h>
#include <android/log.h>

#define LOG_DEBUG(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "BenchmarkDebug", fmt, ##__VA_ARGS__)

#include "benchmark.h"

namespace {

void reportResult(JNIEnv* env, jobject callback, jmethodID method,
                  const std::string& test_name, const std::string& raw_metrics) {
    std::string final_result = test_name + "|" + raw_metrics;
    jstring result_jstr = env->NewStringUTF(final_result.c_str());
    env->CallVoidMethod(callback, method, result_jstr);
    env->DeleteLocalRef(result_jstr);
}

void reportSection(JNIEnv* env, jobject callback, jmethodID method,
                   const std::string& section_name) {
    std::string section = "---" + section_name + "---|0|0|0|0";
    jstring result_jstr = env->NewStringUTF(section.c_str());
    env->CallVoidMethod(callback, method, result_jstr);
    env->DeleteLocalRef(result_jstr);
}

template<typename BenchmarkFn>
void runScalingTest(JNIEnv* env, jobject callback, jmethodID method,
                    const std::string& base_name, BenchmarkFn&& benchmark_fn,
                    unsigned int num_cores) {
    double prev_bandwidth = 0;

    for (unsigned int threads = 2; threads <= num_cores; ) {
        std::string name = base_name + " (" + std::to_string(threads) + " thr)";
        std::string raw = benchmark_fn(threads);
        BenchmarkResult result = parseBenchmarkResult(raw);
        double bandwidth = result.getBandwidthGBps();

        reportResult(env, callback, method, name, raw);

        if (prev_bandwidth > 0) {
            double improvement = (bandwidth - prev_bandwidth) / prev_bandwidth;
            if (improvement < benchmark::kMinImprovementThreshold) {
                break;
            }
        }
        prev_bandwidth = bandwidth;

        if (num_cores > 16) {
            threads *= 2;
        } else {
            threads += 2;
        }
    }
}

}  // namespace

extern "C" JNIEXPORT jint JNICALL
Java_name_duzenko_benchmark_BenchmarkModel_getTestCount(
        JNIEnv* env,
        jobject /* this */) {
    return 20;
}

extern "C" JNIEXPORT void JNICALL
Java_name_duzenko_benchmark_BenchmarkModel_runAllMemoryBenchmarks(
        JNIEnv* env,
        jobject /* this */,
        jobject callback) {

    jclass callbackClass = env->GetObjectClass(callback);
    jmethodID onProgressUpdate = env->GetMethodID(callbackClass, "onProgressUpdate", "(Ljava/lang/String;)V");
    jmethodID onFinished = env->GetMethodID(callbackClass, "onFinished", "()V");

    auto indexed_worker = benchmark::makeIndexedWorker();
    auto memset_worker = benchmark::makeMemsetWorker();

    const unsigned int num_cores = std::thread::hardware_concurrency();

    // Single-threaded tests
    reportSection(env, callback, onProgressUpdate, "Single-threaded");
    reportResult(env, callback, onProgressUpdate, "64-bit", runBenchmark<uint64_t>(1, indexed_worker));

#ifdef HAS_NEON
    reportResult(env, callback, onProgressUpdate, "128-bit NEON", runBenchmark<uint8x16_t>(1, neon_worker));
    reportResult(env, callback, onProgressUpdate, "128-bit NEON NT", runBenchmark<uint8x16_t>(1, neon_stream_worker));
#endif

    reportResult(env, callback, onProgressUpdate, "memset", runBenchmark<uint8_t>(1, memset_worker));

    // Multi-threaded scaling tests
    if (num_cores >= 2) {
        reportSection(env, callback, onProgressUpdate, "Multi-threaded scaling");

        runScalingTest(env, callback, onProgressUpdate, "64-bit",
            [&](unsigned int threads) {
                return runBenchmark<uint64_t>(threads, indexed_worker);
            }, num_cores);

#ifdef HAS_NEON
        runScalingTest(env, callback, onProgressUpdate, "128-bit NEON",
            [&](unsigned int threads) {
                return runBenchmark<uint8x16_t>(threads, neon_worker);
            }, num_cores);

        runScalingTest(env, callback, onProgressUpdate, "128-bit NEON NT",
            [&](unsigned int threads) {
                return runBenchmark<uint8x16_t>(threads, neon_stream_worker);
            }, num_cores);
#endif

        runScalingTest(env, callback, onProgressUpdate, "memset",
            [&](unsigned int threads) {
                return runBenchmark<uint8_t>(threads, memset_worker);
            }, num_cores);
    }

    env->CallVoidMethod(callback, onFinished);
}
