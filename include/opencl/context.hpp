#pragma once

#include "cl.h"
#include <string>

class OpenCLContext {
public:
    OpenCLContext();
    ~OpenCLContext();

    cl_context context() const;
    cl_command_queue queue() const;
    cl_program buildProgramFromFile(const std::string& path);
    cl_kernel createKernel(cl_program program, const std::string& kernel_name);

private:
    cl_platform_id platform_;
    cl_device_id device_;
    cl_context context_;
    cl_command_queue queue_;
};
