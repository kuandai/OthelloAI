#include <gtest/gtest.h>
#include "opencl/context.hpp"
#include "generated/weights.hpp"
#include "generated/kernels.hpp"

// Test kernel: add_numbers.cl
// Adds two float inputs on the GPU using OpenCL
TEST(OpenCLTest, CLAddFloats) {
    using namespace std;

    // Load kernel source code
    std::string source = kernel_sources.at("add_numbers.cl");
    const char* src = source.c_str();
    size_t len = source.size();

    cl_int err;
    OpenCLContext context;

    // Create and build the OpenCL program
    cl_program program = clCreateProgramWithSource(context.context(), 1, &src, &len, &err);
    ASSERT_EQ(err, CL_SUCCESS);
    err = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS);

    // Create the OpenCL kernel from the compiled program
    cl_kernel kernel = clCreateKernel(program, "add_numbers", &err);
    ASSERT_EQ(err, CL_SUCCESS);

    // Allocate and initialize input/output buffers
    float a = 3.0f, b = 4.0f;
    cl_mem a_buf = clCreateBuffer(context.context(), CL_MEM_READ_ONLY, sizeof(float), nullptr, &err);
    cl_mem b_buf = clCreateBuffer(context.context(), CL_MEM_READ_ONLY, sizeof(float), nullptr, &err);
    cl_mem output_buf = clCreateBuffer(context.context(), CL_MEM_WRITE_ONLY, sizeof(float), nullptr, &err);
    ASSERT_EQ(err, CL_SUCCESS);

    // Copy data to device
    err = clEnqueueWriteBuffer(context.queue(), a_buf, CL_TRUE, 0, sizeof(float), &a, 0, nullptr, nullptr);
    err |= clEnqueueWriteBuffer(context.queue(), b_buf, CL_TRUE, 0, sizeof(float), &b, 0, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS);

    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_buf);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &b_buf);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_buf);
    ASSERT_EQ(err, CL_SUCCESS);

    // Run kernel with 1 work-item
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(context.queue(), kernel, 1, nullptr, &global_work_size, nullptr, 0, nullptr, nullptr);
    clFinish(context.queue());
    ASSERT_EQ(err, CL_SUCCESS);

    // Read result back to host
    float result = 0.0f;
    err = clEnqueueReadBuffer(context.queue(), output_buf, CL_TRUE, 0, sizeof(float), &result, 0, nullptr, nullptr);
    ASSERT_EQ(err, CL_SUCCESS);

    // Verify result is correct
    EXPECT_FLOAT_EQ(result, 7.0f);

    // Cleanup OpenCL resources
    clReleaseMemObject(a_buf);
    clReleaseMemObject(b_buf);
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
}
