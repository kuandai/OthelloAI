#include "model/inference_model.hpp"
#include "generated/weights.hpp"
#include "generated/kernels.hpp"

#include <cassert>
#include <cstring>

InferenceModel::InferenceModel() {
    std::string source = kernel_sources.at("add_numbers.cl");
    const char* src = source.c_str();
    size_t len = source.size();

    // Create and build the CL program
    cl_int err;
    program_ = clCreateProgramWithSource(context_.context(), 1, &src, &len, &err);
    err = clBuildProgram(program_, 0, nullptr, nullptr, nullptr, nullptr);

    // Create the kernel
    kernel_ = clCreateKernel(program_, "add_numbers", &err);

    // Allocate the buffers
    a_buf_ = clCreateBuffer(context_.context(), CL_MEM_READ_ONLY, sizeof(float), nullptr, &err);
    b_buf_ = clCreateBuffer(context_.context(), CL_MEM_READ_ONLY, sizeof(float), nullptr, &err);
    output_buf_ = clCreateBuffer(context_.context(), CL_MEM_WRITE_ONLY, sizeof(float), nullptr, &err);
}

float InferenceModel::add(float a, float b) {
    cl_int err;

    // Write inputs
    err = clEnqueueWriteBuffer(context_.queue(), a_buf_, CL_TRUE, 0, sizeof(float), &a, 0, nullptr, nullptr);
    err |= clEnqueueWriteBuffer(context_.queue(), b_buf_, CL_TRUE, 0, sizeof(float), &b, 0, nullptr, nullptr);
    assert(err == CL_SUCCESS);

    // Set args
    err = clSetKernelArg(kernel_, 0, sizeof(cl_mem), &a_buf_);
    err |= clSetKernelArg(kernel_, 1, sizeof(cl_mem), &b_buf_);
    err |= clSetKernelArg(kernel_, 2, sizeof(cl_mem), &output_buf_);
    assert(err == CL_SUCCESS);

    // Launch
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(context_.queue(), kernel_, 1, nullptr, &global_work_size, nullptr, 0, nullptr, nullptr);
    clFinish(context_.queue());
    assert(err == CL_SUCCESS);

    // Read result
    float result;
    err = clEnqueueReadBuffer(context_.queue(), output_buf_, CL_TRUE, 0, sizeof(float), &result, 0, nullptr, nullptr);
    assert(err == CL_SUCCESS);

    return result;
}
