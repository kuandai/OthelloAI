#include "opencl/context.hpp"
#include <fstream>
#include <iostream>
#include <vector>

#include <spdlog/spdlog.h>

OpenCLContext::OpenCLContext() {
    cl_int err;

    // Platform
    cl_uint num_platforms;
    clGetPlatformIDs(0, nullptr, &num_platforms);
    std::vector<cl_platform_id> platforms(num_platforms);
    clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
    platform_ = platforms[0];

    // Device
    cl_uint num_devices;
    clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
    std::vector<cl_device_id> devices(num_devices);
    clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, num_devices, devices.data(), nullptr);
    device_ = devices[0];

    // Context
    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
    spdlog::info("OpenCL context created successfully");

    // Command queue
    #ifdef CL_VERSION_2_0
    queue_ = clCreateCommandQueueWithProperties(context_, device_, nullptr, &err);
    #else
    queue_ = clCreateCommandQueue(context_, device_, 0, &err);
    #endif
}

OpenCLContext::~OpenCLContext() {
    clReleaseCommandQueue(queue_);
    clReleaseContext(context_);
}

cl_context OpenCLContext::context() const { return context_; }
cl_command_queue OpenCLContext::queue() const { return queue_; }

cl_program OpenCLContext::buildProgramFromFile(const std::string& path) {
    std::ifstream file(path);
    std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const char* src_cstr = src.c_str();
    size_t length = src.size();

    cl_int err;
    cl_program program = clCreateProgramWithSource(context_, 1, &src_cstr, &length, &err);
    err = clBuildProgram(program, 1, &device_, nullptr, nullptr, nullptr);

    if (err != CL_SUCCESS) {
        // Print build log
        char buffer[2048];
        clGetProgramBuildInfo(program, device_, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, nullptr);
        std::cerr << "OpenCL build error:\n" << buffer << std::endl;
    }

    return program;
}

cl_kernel OpenCLContext::createKernel(cl_program program, const std::string& kernel_name) {
    cl_int err;
    cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &err);
    return kernel;
}
