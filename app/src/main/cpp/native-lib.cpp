#include <jni.h>
#include <android/log.h>
#include <thread>

#define LOG_DEBUG(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "BenchmarkDebug", fmt, ##__VA_ARGS__)

#include "benchmark.h"
#include "simd_test.h"

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

extern "C" JNIEXPORT jint JNICALL
Java_name_duzenko_benchmark_BenchmarkModel_getHardwareConcurrency(
        JNIEnv* env,
        jobject /* this */) {
    return std::thread::hardware_concurrency();
}

extern "C" JNIEXPORT void JNICALL
Java_name_duzenko_benchmark_BenchmarkModel_runAllMemoryBenchmarks(
        JNIEnv* env,
        jobject /* this */,
        jobject callback) {

#if defined(__i386__) || defined(__x86_64__)
    test_sse_addition();
    const auto& features = cpu_features::X86Features::get();
    LOG_DEBUG("CPU Features: SSE2=%d, AVX=%d, AVX2=%d, AVX512F=%d",
              features.has_sse2, features.has_avx, features.has_avx2, features.has_avx512f);
#endif

    jclass callbackClass = env->GetObjectClass(callback);
    jmethodID onProgressUpdate = env->GetMethodID(callbackClass, "onProgressUpdate", "(Ljava/lang/String;)V");
    jmethodID onFinished = env->GetMethodID(callbackClass, "onFinished", "()V");

    auto indexed_worker = benchmark::makeIndexedWorker();
    auto memset_worker = benchmark::makeMemsetWorker();

    const unsigned int num_cores = std::thread::hardware_concurrency();

    // Single-threaded tests
    reportSection(env, callback, onProgressUpdate, "Single-threaded");
    reportResult(env, callback, onProgressUpdate, "32-bit", runBenchmark<uint32_t>(1, indexed_worker));
    reportResult(env, callback, onProgressUpdate, "64-bit", runBenchmark<uint64_t>(1, indexed_worker));

#if defined(__i386__) || defined(__x86_64__)
    if (features.has_sse2) {
#if defined(__x86_64__)
        reportResult(env, callback, onProgressUpdate, "64-bit NT", runBenchmark<int64_t>(1, scalar_stream_worker));
#else
        reportResult(env, callback, onProgressUpdate, "32-bit NT", runBenchmark<int32_t>(1, scalar_stream_worker_32));
#endif
        reportResult(env, callback, onProgressUpdate, "128-bit SSE", runBenchmark<__m128i>(1, sse_worker));
        reportResult(env, callback, onProgressUpdate, "128-bit SSE NT", runBenchmark<__m128i>(1, sse_stream_worker));
    }
    if (features.has_avx2) {
        reportResult(env, callback, onProgressUpdate, "256-bit AVX2", runBenchmark<__m256i>(1, avx_worker));
        reportResult(env, callback, onProgressUpdate, "256-bit AVX2 NT", runBenchmark<__m256i>(1, avx_stream_worker));
    }
    if (features.has_avx512f) {
        reportResult(env, callback, onProgressUpdate, "512-bit AVX512", runBenchmark<__m512i>(1, avx512_worker));
        reportResult(env, callback, onProgressUpdate, "512-bit AVX512 NT", runBenchmark<__m512i>(1, avx512_stream_worker));
    }
#endif

#ifdef HAS_NEON
    reportResult(env, callback, onProgressUpdate, "128-bit NEON", runBenchmark<uint8x16_t>(1, neon_worker));
    reportResult(env, callback, onProgressUpdate, "128-bit NEON NT", runBenchmark<uint8x16_t>(1, neon_stream_worker));
#endif

    reportResult(env, callback, onProgressUpdate, "memset", runBenchmark<uint8_t>(1, memset_worker));

    // Multi-threaded scaling tests
    if (num_cores >= 2) {
        reportSection(env, callback, onProgressUpdate, "Multi-threaded scaling");

        runScalingTest(env, callback, onProgressUpdate, "32-bit",
            [&](unsigned int threads) {
                return runBenchmark<uint32_t>(threads, indexed_worker);
            }, num_cores);

        runScalingTest(env, callback, onProgressUpdate, "64-bit",
            [&](unsigned int threads) {
                return runBenchmark<uint64_t>(threads, indexed_worker);
            }, num_cores);

#if defined(__i386__) || defined(__x86_64__)
        if (features.has_sse2) {
#if defined(__x86_64__)
            runScalingTest(env, callback, onProgressUpdate, "64-bit NT",
                [&](unsigned int threads) {
                    return runBenchmark<int64_t>(threads, scalar_stream_worker);
                }, num_cores);
#else
            runScalingTest(env, callback, onProgressUpdate, "32-bit NT",
                [&](unsigned int threads) {
                    return runBenchmark<int32_t>(threads, scalar_stream_worker_32);
                }, num_cores);
#endif
            runScalingTest(env, callback, onProgressUpdate, "128-bit SSE",
                [&](unsigned int threads) {
                    return runBenchmark<__m128i>(threads, sse_worker);
                }, num_cores);
            runScalingTest(env, callback, onProgressUpdate, "128-bit SSE NT",
                [&](unsigned int threads) {
                    return runBenchmark<__m128i>(threads, sse_stream_worker);
                }, num_cores);
        }
        if (features.has_avx2) {
            runScalingTest(env, callback, onProgressUpdate, "256-bit AVX2",
                [&](unsigned int threads) {
                    return runBenchmark<__m256i>(threads, avx_worker);
                }, num_cores);
            runScalingTest(env, callback, onProgressUpdate, "256-bit AVX2 NT",
                [&](unsigned int threads) {
                    return runBenchmark<__m256i>(threads, avx_stream_worker);
                }, num_cores);
        }
        if (features.has_avx512f) {
            runScalingTest(env, callback, onProgressUpdate, "512-bit AVX512",
                [&](unsigned int threads) {
                    return runBenchmark<__m512i>(threads, avx512_worker);
                }, num_cores);
            runScalingTest(env, callback, onProgressUpdate, "512-bit AVX512 NT",
                [&](unsigned int threads) {
                    return runBenchmark<__m512i>(threads, avx512_stream_worker);
                }, num_cores);
        }
#endif

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
