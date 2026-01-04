#ifndef SIMD_TEST_H
#define SIMD_TEST_H

#if defined(__i386__) || defined(__x86_64__)
void test_sse_addition();
#endif

#endif // SIMD_TEST_H
