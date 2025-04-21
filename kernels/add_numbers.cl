__kernel void add_numbers(__global const float* a,
                          __global const float* b,
                          __global float* result) {
    result[0] = a[0] + b[0];
}
