#pragma once

#include "tamm/errors.hpp"
#include <array>
#include <memory>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#if defined(USE_CUDA)
#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#elif defined(USE_HIP)
#include <hip/hip_runtime.h>
#include <rocblas/rocblas.h>
#elif defined(USE_DPCPP)
#include "sycl_device.hpp"
#include <oneapi/mkl/blas.hpp>
#endif

namespace tamm {

class GPUStreamPool;

#if defined(USE_HIP)
using gpuStream_t   = std::pair<hipStream_t, rocblas_handle>;
using gpuEvent_t    = hipEvent_t;
using gpuMemcpyKind = hipMemcpyKind;
#define gpuMemcpyHostToDevice hipMemcpyHostToDevice
#define gpuMemcpyDeviceToHost hipMemcpyDeviceToHost
#define gpuMemcpyDeviceToDevice hipMemcpyDeviceToDevice

#define HIP_CHECK(FUNC)                                                                           \
  do {                                                                                            \
    hipError_t err_ = (FUNC);                                                                     \
    if(err_ != hipSuccess) {                                                                      \
      std::ostringstream msg;                                                                     \
      msg << "HIP Error: " << hipGetErrorString(err_) << ", at " << __FILE__ << " : " << __LINE__ \
          << std::endl;                                                                           \
      throw std::runtime_error(msg.str());                                                        \
    }                                                                                             \
  } while(0)

#define ROCBLAS_CHECK(FUNC)                                                                      \
  do {                                                                                           \
    rocblas_status err_ = (FUNC);                                                                \
    if(err_ != rocblas_status_success) {                                                         \
      std::ostringstream msg;                                                                    \
      msg << "ROCBLAS Error: " << rocblas_status_to_string(err_) << ", at " << __FILE__ << " : " \
          << __LINE__ << std::endl;                                                              \
      throw std::runtime_error(msg.str());                                                       \
    }                                                                                            \
  } while(0)
#endif // USE_HIP

#if defined(USE_CUDA)
using gpuStream_t   = std::pair<cudaStream_t, cublasHandle_t>;
using gpuEvent_t    = cudaEvent_t;
using gpuMemcpyKind = cudaMemcpyKind;
#define gpuMemcpyHostToDevice cudaMemcpyHostToDevice
#define gpuMemcpyDeviceToHost cudaMemcpyDeviceToHost
#define gpuMemcpyDeviceToDevice cudaMemcpyDeviceToDevice

#define CUDA_CHECK(FUNC)                                                                \
  do {                                                                                  \
    cudaError_t err_ = (FUNC);                                                          \
    if(err_ != cudaSuccess) {                                                           \
      std::ostringstream msg;                                                           \
      msg << "CUDA Error: " << cudaGetErrorString(err_) << ", at " << __FILE__ << " : " \
          << __LINE__ << std::endl;                                                     \
      throw std::runtime_error(msg.str());                                              \
    }                                                                                   \
  } while(0)

#define CUBLAS_CHECK(FUNC)                                                                   \
  do {                                                                                       \
    cublasStatus_t err_ = (FUNC);                                                            \
    if(err_ != CUBLAS_STATUS_SUCCESS) {                                                      \
      std::ostringstream msg;                                                                \
      msg << "CUBLAS Error: " << cublasGetStatusString(err_) << ", at " << __FILE__ << " : " \
          << __LINE__ << std::endl;                                                          \
      throw std::runtime_error(msg.str());                                                   \
    }                                                                                        \
  } while(0)
#endif // USE_CUDA

#if defined(USE_DPCPP)
using gpuStream_t   = std::pair<sycl::queue, std::nullopt_t>;
using gpuEvent_t    = sycl::event;
using gpuMemcpyKind = int;
#define gpuMemcpyHostToDevice 0
#define gpuMemcpyDeviceToHost 1
#define gpuMemcpyDeviceToDevice 2

auto sycl_asynchandler = [](sycl::exception_list exceptions) {
  for(std::exception_ptr const& e: exceptions) {
    try {
      std::rethrow_exception(e);
    } catch(sycl::exception const& ex) {
      std::cout << "Caught asynchronous SYCL exception:" << std::endl
                << ex.what() << ", SYCL code: " << ex.code() << std::endl;
    }
  }
};
#endif // USE_DPCPP

static inline void getDeviceCount(int* id) {
#if defined(USE_CUDA)
  CUDA_CHECK(cudaGetDeviceCount(id));
#elif defined(USE_HIP)
  HIP_CHECK(hipGetDeviceCount(id));
#elif defined(USE_DPCPP)
  syclGetDeviceCount(id);
#endif
}

// The following API is to get the hardware count of
// GPUs/GCDs/Xe-stacks/tiles on a given node. Unlike the
// above API, this method is not affected by the masking
// env variables like CUDA/ROCR_VISIBLE_DEVICES or ZE_AFFINITY_MASK
static inline void getHardwareGPUCount(int* gpus_per_node) {
  std::array<char, 128> buffer;
  std::string           result, m_call;

#if defined(USE_CUDA)
  m_call = "nvidia-smi --query-gpu=name --format=csv,noheader | wc -l";
#elif defined(USE_HIP)
  m_call = "rocm-smi -i | grep GPU | wc -l";
#elif defined(USE_DPCPP)
  sycl::platform pltf = sycl_get_device(0)->get_platform();
  if(pltf.get_backend() == sycl::backend::ext_oneapi_level_zero ||
     pltf.get_backend() == sycl::backend::opencl) {
    m_call = "cat /sys/class/drm/card*/gt/gt*/id | wc -l";
  }
  else if(pltf.get_backend() == sycl::backend::ext_oneapi_cuda) {
    m_call = "nvidia-smi --query-gpu=name --format=csv,noheader | wc -l";
  }
  else if(pltf.get_backend() == sycl::backend::ext_oneapi_hip) {
    m_call = "rocm-smi -i | grep GPU | wc -l";
  }
#endif

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(m_call.c_str(), "r"), pclose);
  if(!pipe) { throw std::runtime_error("popen() failed!"); }
  while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) { result += buffer.data(); }
  *gpus_per_node = stoi(result);
}

static inline std::string getDeviceName() {
#if defined(USE_CUDA)
  cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, 0);
  return prop.name;
#elif defined(USE_HIP)
  hipDeviceProp_t prop;
  hipGetDeviceProperties(&prop, 0);
  return prop.name;
#elif defined(USE_DPCPP)
  return sycl_get_device(0)->get_info<sycl::info::device::name>();
#endif
}

static inline std::string getRuntimeVersion() {
#if defined(USE_CUDA)
  int cudaVersion;
  cudaRuntimeGetVersion(&cudaVersion);
  std::string cudav =
    std::to_string(cudaVersion / 1000) + '.' + std::to_string((cudaVersion % 1000) / 10);
  return "CUDA v" + cudav;
#elif defined(USE_HIP)
  int hipVersion;
  hipRuntimeGetVersion(&hipVersion);
  std::string hipv = std::to_string(hipVersion / 10000000) + '.' +
                     std::to_string(hipVersion / 100000 % 100) + '.' +
                     std::to_string(hipVersion % 100000);
  return "ROCM v" + hipv;
#elif defined(USE_DPCPP)
  return "SYCL v" + sycl_get_device(0)->get_info<sycl::info::device::driver_version>();
#endif
}

static inline void gpuMemGetInfo(size_t* free, size_t* total) {
#if defined(USE_CUDA)
  cudaMemGetInfo(free, total);
#elif defined(USE_HIP)
  hipMemGetInfo(free, total);
#elif defined(USE_DPCPP)
  syclMemGetInfo(free, total);
#endif
}

static inline void gpuSetDevice(int active_device) {
#ifdef USE_CUDA
  CUDA_CHECK(cudaSetDevice(active_device));
#elif defined(USE_HIP)
  HIP_CHECK(hipSetDevice(active_device));
#elif defined(USE_DPCPP)
  syclSetDevice(active_device);
#endif
}

static inline void gpuGetDevice(int* active_device) {
#ifdef USE_CUDA
  CUDA_CHECK(cudaGetDevice(active_device));
#elif defined(USE_HIP)
  HIP_CHECK(hipGetDevice(active_device));
#elif defined(USE_DPCPP)
  syclGetDevice(active_device);
#endif
}

// ABB (11/20/23): Added to be used by the CCSD-(T) kernel
// template<typename T>
// static void gpuMemcpyToSymbolAsync(T* dst, const T* src, size_t count, size_t offset,
//                                    gpuMemcpyKind kind, gpuStream_t& stream) {
// #if defined(USE_DPCPP)
//   stream.first.memcpy(sycl::ext::oneapi::experimental::device_global<T,
//   decltype(sycl::ext::oneapi::experimental::properties(sycl::ext::oneapi::experimental::device_image_scope))>(dst),
//   src, count * sizeof(T), 0, {});
// #elif defined(USE_CUDA)
//   CUDA_CHECK(cudaMemcpyToSymbolAsync(dst, src, count * sizeof(T), 0, kind, stream.first));
// #elif defined(USE_HIP)
//   HIP_CHECK(hipMemcpyToSymbolAsync(dst, src, count * sizeof(T), 0, kind, stream.first));
// #endif
// }

template<typename T>
static void gpuMemcpyAsync(T* dst, const T* src, size_t count, gpuMemcpyKind kind,
                           gpuStream_t& stream) {
#if defined(USE_DPCPP)
  if(kind == gpuMemcpyDeviceToDevice) { stream.first.copy(src, dst, count); }
  else { stream.first.memcpy(dst, src, count * sizeof(T)); }
#elif defined(USE_CUDA)
  CUDA_CHECK(cudaMemcpyAsync(dst, src, count * sizeof(T), kind, stream.first));
#elif defined(USE_HIP)
  HIP_CHECK(hipMemcpyAsync(dst, src, count * sizeof(T), kind, stream.first));
#endif
}

static inline void gpuMemsetAsync(void*& ptr, size_t sizeInBytes, gpuStream_t stream) {
#if defined(USE_DPCPP)
  stream.first.memset(ptr, 0, sizeInBytes);
#elif defined(USE_HIP)
  hipMemsetAsync(ptr, 0, sizeInBytes, stream.first);
#elif defined(USE_CUDA)
  cudaMemsetAsync(ptr, 0, sizeInBytes, stream.first);
#endif
}

static inline void gpuStreamSynchronize(gpuStream_t stream) {
#if defined(USE_DPCPP)
  if(!stream.first.ext_oneapi_empty()) stream.first.wait();
#elif defined(USE_HIP)
  hipStreamSynchronize(stream.first);
#elif defined(USE_CUDA)
  cudaStreamSynchronize(stream.first);
#endif
}

static inline void gpuEventRecord(gpuEvent_t& event, gpuStream_t stream) {
#if defined(USE_DPCPP)
  event = stream.first.ext_oneapi_submit_barrier();
#elif defined(USE_HIP)
  hipEventRecord(event, stream.first);
#elif defined(USE_CUDA)
  cudaEventRecord(event, stream.first);
#endif
}

static inline void gpuEventCreateWithFlags(gpuEvent_t* event) {
#if defined(USE_HIP)
  hipEventCreateWithFlags(event, hipEventDisableTiming);
#elif defined(USE_CUDA)
  cudaEventCreateWithFlags(event, cudaEventDisableTiming);
#endif
}

static inline void gpuEventDestroy(gpuEvent_t& event) {
#if defined(USE_HIP)
  hipEventDestroy(event);
#elif defined(USE_CUDA)
  cudaEventDestroy(event);
#endif
}

static inline void gpuEventSynchronize(gpuEvent_t& event) {
#if defined(USE_DPCPP)
  event.wait();
#elif defined(USE_HIP)
  hipEventSynchronize(event);
#elif defined(USE_CUDA)
  cudaEventSynchronize(event);
#endif
}

static inline bool gpuEventQuery(gpuEvent_t& event) {
#if defined(USE_DPCPP)
  return (event.get_info<sycl::info::event::command_execution_status>() ==
          sycl::info::event_command_status::complete);
#elif defined(USE_HIP)
  return (hipEventQuery(event) == hipSuccess);
#elif defined(USE_CUDA)
  return (cudaEventQuery(event) == cudaSuccess);
#endif
}

class GPUStreamPool {
protected:
  int                      default_deviceID{0};
  uint32_t                 nstreams{1};
  uint32_t                 streamCount{0};
  std::vector<gpuStream_t> _devStream;

private:
  GPUStreamPool() {
    // Assert here if multi-GPUs are detected
    // int ngpus{0};
    // getDeviceCount(&ngpus);
    // if (ngpus > 1) { tamm_terminate("[TAMM_ERROR] Multiple (" + std::to_string(ngpus) + ")GPUs
    // bound per rank!"); gpuSetDevice(default_deviceID);

    gpuGetDevice(&default_deviceID);

    for(uint32_t j = 0; j < nstreams; j++) {
#if defined(USE_CUDA)
      cudaStream_t gpu_stream;
      CUDA_CHECK(cudaStreamCreateWithFlags(&gpu_stream, cudaStreamNonBlocking));

      cublasHandle_t gpu_blashandle;
      CUBLAS_CHECK(cublasCreate(&gpu_blashandle));
      CUBLAS_CHECK(cublasSetStream(gpu_blashandle, gpu_stream));

      _devStream.push_back(std::make_pair(gpu_stream, gpu_blashandle));
#elif defined(USE_HIP)
      hipStream_t gpu_stream;
      HIP_CHECK(hipStreamCreateWithFlags(&gpu_stream, hipStreamNonBlocking));

      rocblas_handle gpu_blashandle;
      ROCBLAS_CHECK(rocblas_create_handle(&gpu_blashandle));
      ROCBLAS_CHECK(rocblas_set_stream(gpu_blashandle, gpu_stream));

      _devStream.push_back(std::make_pair(gpu_stream, gpu_blashandle));
#elif defined(USE_DPCPP)
      _devStream.push_back(std::make_pair(
        sycl::queue(*sycl_get_context(default_deviceID), *sycl_get_device(default_deviceID),
                    sycl_asynchandler, sycl::property_list{sycl::property::queue::in_order{}}),
        std::nullopt));
#endif
    }
  }

  ~GPUStreamPool() {
    for(uint32_t j = 0; j < nstreams; j++) {
#if defined(USE_CUDA)
      cudaStreamDestroy(_devStream[j].first);
      cublasDestroy(_devStream[j].second);
#elif defined(USE_HIP)
      hipStreamDestroy(_devStream[j].first);
      rocblas_destroy_handle(_devStream[j].second);
#endif
    }
  }

public:
  /// Returns a GPU stream
  gpuStream_t& getStream() { return _devStream[0]; }
  /// Returns a round-robin GPU stream
  gpuStream_t& getRRStream() { return _devStream[streamCount++ % nstreams]; }
  /// Returns all GPU stream
  std::vector<gpuStream_t>& getAllStream() { return _devStream; }

  /// Returns the instance of device manager singleton.
  inline static GPUStreamPool& getInstance() {
    static GPUStreamPool d_m{};
    return d_m;
  }

  GPUStreamPool(const GPUStreamPool&)            = delete;
  GPUStreamPool& operator=(const GPUStreamPool&) = delete;
  GPUStreamPool(GPUStreamPool&&)                 = delete;
  GPUStreamPool& operator=(GPUStreamPool&&)      = delete;
};

static inline void gpuDeviceSynchronize() {
#if defined(USE_DPCPP)
  auto streams = tamm::GPUStreamPool::getInstance().getAllStream();
  for(auto& str: streams) { str.first.wait(); }
#elif defined(USE_HIP)
  hipDeviceSynchronize();
#elif defined(USE_CUDA)
  cudaDeviceSynchronize();
#endif
}

static inline void* getPinnedMem(size_t bytes) {
  void* ptr = nullptr;
#if defined(USE_CUDA)
  cudaMallocHost((void**) &ptr, bytes);
#elif defined(USE_HIP)
  hipMallocHost((void**) &ptr, bytes);
#elif defined(USE_DPCPP)
  ptr = (void*) sycl::malloc_host(bytes, tamm::GPUStreamPool::getInstance().getStream().first);
#endif
  return ptr;
}

static inline void freePinnedMem(void* ptr) {
#if defined(USE_CUDA)
  cudaFreeHost(ptr);
#elif defined(USE_HIP)
  hipFreeHost(ptr);
#elif defined(USE_DPCPP)
  sycl::free(ptr, tamm::GPUStreamPool::getInstance().getStream().first);
#endif
}

// This API needs to be defined after the class GPUStreamPool since the classs
// is only declared and defined before this method
} // namespace tamm
