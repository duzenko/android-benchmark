# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("myapplication")

# Check if building for Android or desktop
if(ANDROID)
    # Android build: Create shared library for JNI
    add_library(${CMAKE_PROJECT_NAME} SHARED
            native-lib.cpp
            simd_test.cpp)

    # Specifies libraries CMake should link to your target library. You
    # can link libraries from various origins, such as libraries defined in this
    # build script, prebuilt third-party libraries, or Android system libraries.
    target_link_libraries(${CMAKE_PROJECT_NAME}
            android
            log)
else()
    # Desktop build: Create console executable
    add_executable(benchmark main.cpp)

    # Enable C++17 for better compatibility
    set_target_properties(benchmark PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON)

    # --- Enable SIMD for desktop build ---
    include(CheckCXXCompilerFlag)

    if(MSVC)
        check_cxx_compiler_flag("/arch:SSE2" HAS_SSE2_FLAG)
        if(HAS_SSE2_FLAG)
            target_compile_options(benchmark PRIVATE /arch:SSE2)
            target_compile_definitions(benchmark PRIVATE HAS_SSE2)
        endif()

        check_cxx_compiler_flag("/arch:AVX" HAS_AVX_FLAG)
        if(HAS_AVX_FLAG)
            target_compile_options(benchmark PRIVATE /arch:AVX)
            target_compile_definitions(benchmark PRIVATE HAS_AVX_RUNTIME_CHECK)
        endif()

        check_cxx_compiler_flag("/arch:AVX2" HAS_AVX2_FLAG)
        if(HAS_AVX2_FLAG)
            target_compile_options(benchmark PRIVATE /arch:AVX2)
            target_compile_definitions(benchmark PRIVATE HAS_AVX2_RUNTIME_CHECK)
        endif()

        check_cxx_compiler_flag("/arch:AVX512F" HAS_AVX512_FLAG)
        if(HAS_AVX512_FLAG)
            target_compile_options(benchmark PRIVATE /arch:AVX512F)
            target_compile_definitions(benchmark PRIVATE HAS_AVX512_RUNTIME_CHECK)
        endif()
    else()
        check_cxx_compiler_flag("-msse2" HAS_SSE2_FLAG)
        if(HAS_SSE2_FLAG)
            target_compile_options(benchmark PRIVATE -msse2)
            target_compile_definitions(benchmark PRIVATE HAS_SSE2)
        endif()

        check_cxx_compiler_flag("-mavx" HAS_AVX_FLAG)
        if(HAS_AVX_FLAG)
            target_compile_options(benchmark PRIVATE -mavx)
            target_compile_definitions(benchmark PRIVATE HAS_AVX_RUNTIME_CHECK)
        endif()

        check_cxx_compiler_flag("-mavx2" HAS_AVX2_FLAG)
        if(HAS_AVX2_FLAG)
            target_compile_options(benchmark PRIVATE -mavx2)
            target_compile_definitions(benchmark PRIVATE HAS_AVX2_RUNTIME_CHECK)
        endif()

        check_cxx_compiler_flag("-mavx512f" HAS_AVX512_FLAG)
        if(HAS_AVX512_FLAG)
            target_compile_options(benchmark PRIVATE -mavx512f)
            target_compile_definitions(benchmark PRIVATE HAS_AVX512_RUNTIME_CHECK)
        endif()
    endif()
    # --- End SIMD for desktop build ---

    # Link pthread on Unix systems
    if(UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(benchmark Threads::Threads)
    endif()
endif()
