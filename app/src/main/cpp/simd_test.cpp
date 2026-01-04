#include <cassert>

#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h> // For SSE

// Function to test basic SSE functionality
void test_sse_addition() {
    // Create two 4-element float vectors
    __m128 a = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 b = _mm_set_ps(5.0f, 6.0f, 7.0f, 8.0f);

    // Perform SIMD addition
    __m128 result = _mm_add_ps(a, b);

    // Store the result in an array to check the values
    float out[4];
    _mm_storeu_ps(out, result);

    // Check that the addition was successful
    // Note: _mm_set_ps stores values in reverse order in memory
    assert(out[0] == 12.0f); // 4.0 + 8.0
    assert(out[1] == 10.0f); // 3.0 + 7.0
    assert(out[2] == 8.0f);  // 2.0 + 6.0
    assert(out[3] == 6.0f);  // 1.0 + 5.0
}

#endif // defined(__i386__) || defined(__x86_64__)
