#include <spdlog/spdlog.h>
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <CL/cl.h>

int main() {
    cl_uint platformCount = 0;
    clGetPlatformIDs(0, nullptr, &platformCount);
    spdlog::info("OpenCL platform count: {}", platformCount);

    return 0;
}   
