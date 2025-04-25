#include <gtest/gtest.h>
#include "replay/buffer.hpp"

TEST(ReplayBufferTest, InsertAndSample) {
    ReplayBuffer buffer(5);

    for (int i = 0; i < 10; ++i) {
        TrainSample s;
        s.state = std::vector<float>(192, float(i)); // fill with i
        s.policy = std::vector<float>(64, 1.0f / 64);
        s.value = (i % 3) - 1;
        buffer.insert(s);
    }

    EXPECT_EQ(buffer.size(), 5);

    auto batch = buffer.sample(3);
    EXPECT_EQ(batch.size(), 3);
    for (const auto& s : batch) {
        EXPECT_EQ(s.state.size(), 192);
        EXPECT_EQ(s.policy.size(), 64);
        EXPECT_GE(s.value, -1.0f);
        EXPECT_LE(s.value, 1.0f);
    }
}
