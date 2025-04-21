#include <gtest/gtest.h>
#include "model/inference_model.hpp"

TEST(OpenCLTest, CLAddFloats) {
    InferenceModel model;
    float result = model.add(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(result, 7.0f);
}