# NCU Profiling Error Code 11 - Troubleshooting Guide

## Problem
You encountered error code 11 when running NVIDIA Nsight Compute (NCU) profiler:
```bash
sudo ./ncu ~/work/How_to_optimize_in_GPU/test
==PROF== Connected to process 6719 (/home/lummy/work/How_to_optimize_in_GPU/test)
==ERROR== The application returned an error code (11).
```

## Root Cause
Error code 11 indicates a **segmentation fault (SIGSEGV)**, which typically occurs due to:
1. **Memory access violations** in your application
2. **Race conditions** triggered by the profiler's instrumentation
3. **Profiler instrumentation bugs** when collecting certain metrics
4. **Missing GPU code** - The test executable doesn't contain CUDA kernels

## Solutions

### 1. **Immediate Fix - Use the Correct Test Executable**
The path `~/work/How_to_optimize_in_GPU/test` doesn't exist. Use the test executable we just built:

```bash
# From the build directory
cd /workspace/build
sudo ncu ./test/runTests
```

### 2. **If Still Getting Error Code 11, Try These Steps:**

#### A. Disable Software Pre-Pass
```bash
export NV_COMPUTE_PROFILER_DISABLE_SW_PRE_PASS=1
sudo -E ncu ./test/runTests
```

#### B. Use Basic Profiling Mode
```bash
sudo ncu --set basic ./test/runTests
```

#### C. Profile Specific Kernels Only
```bash
sudo ncu --kernel-regex ".*" --launch-skip 0 --launch-count 1 ./test/runTests
```

#### D. Use Compute Sanitizer First
```bash
# Install CUDA toolkit if not already installed
sudo apt install nvidia-cuda-toolkit

# Run compute sanitizer to detect memory issues
compute-sanitizer ./test/runTests
```

### 3. **For GPU Applications with CUDA Kernels**

If you have a GPU application with CUDA code, create a simple test:

```cpp
// simple_gpu_test.cu
#include <cuda_runtime.h>
#include <iostream>

__global__ void simple_kernel(float* data, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        data[idx] = data[idx] * 2.0f;
    }
}

int main() {
    const int N = 1024;
    float* h_data = new float[N];
    float* d_data;
    
    // Initialize data
    for (int i = 0; i < N; i++) {
        h_data[i] = static_cast<float>(i);
    }
    
    // Allocate GPU memory
    cudaMalloc(&d_data, N * sizeof(float));
    cudaMemcpy(d_data, h_data, N * sizeof(float), cudaMemcpyHostToDevice);
    
    // Launch kernel
    dim3 block(256);
    dim3 grid((N + block.x - 1) / block.x);
    simple_kernel<<<grid, block>>>(d_data, N);
    
    // Copy back and cleanup
    cudaMemcpy(h_data, d_data, N * sizeof(float), cudaMemcpyDeviceToHost);
    cudaFree(d_data);
    
    // Verify results
    std::cout << "First few results: ";
    for (int i = 0; i < 5; i++) {
        std::cout << h_data[i] << " ";
    }
    std::cout << std::endl;
    
    delete[] h_data;
    return 0;
}
```

Compile and profile:
```bash
nvcc -o simple_gpu_test simple_gpu_test.cu
sudo ncu ./simple_gpu_test
```

### 4. **Alternative Profiling Tools**

If NCU continues to fail, try these alternatives:

#### A. NVIDIA Nsight Systems (nsys)
```bash
sudo nsys profile ./test/runTests
```

#### B. Valgrind (for CPU memory issues)
```bash
valgrind --tool=memcheck ./test/runTests
```

#### C. GDB for debugging
```bash
gdb ./test/runTests
(gdb) run
# If it crashes, use 'bt' for backtrace
```

### 5. **Environment Setup Verification**

Ensure your environment is properly configured:

```bash
# Check NVIDIA driver
nvidia-smi

# Check CUDA installation
nvcc --version

# Check NCU installation
ncu --version

# Verify GPU accessibility
nvidia-smi -q | grep "GPU 0"
```

### 6. **Common Issues and Fixes**

#### Issue: No GPU detected
**Solution:** Ensure NVIDIA drivers and CUDA toolkit are properly installed

#### Issue: Permission denied
**Solution:** Run with sudo or add user to appropriate groups:
```bash
sudo usermod -a -G nvidia-smi $USER
```

#### Issue: Profiler hangs
**Solution:** Use timeout and basic profiling:
```bash
timeout 30s sudo ncu --set basic ./your_app
```

## Summary

The error code 11 you encountered is likely due to:
1. **Wrong executable path** - The path `~/work/How_to_optimize_in_GPU/test` doesn't exist
2. **No GPU kernels** - The current test executable doesn't contain CUDA code to profile

**Recommended next steps:**
1. Use the correct path: `/workspace/build/test/runTests`
2. If you need to profile GPU code, create a CUDA application
3. Try the troubleshooting steps above if issues persist

The test executable we built (`runTests`) is a CPU-only application testing a reactive programming library, so NCU profiling may not be meaningful for this particular executable unless it contains GPU kernels.