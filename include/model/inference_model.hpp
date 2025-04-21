#pragma once

#include "othello/board.hpp"
#include "opencl/context.hpp"
#include <vector>

class InferenceModel {
public:
    InferenceModel();

    // Test program
    float add(float a, float b);
private:
    OpenCLContext context_;
    cl_program program_;
    cl_kernel kernel_;
    cl_mem a_buf_, b_buf_, output_buf_;
};
