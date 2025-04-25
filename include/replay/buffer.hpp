#pragma once
#include <vector>
#include <deque>
#include <random>

struct TrainSample {
    std::vector<float> state;    // 3x8x8 = 192
    std::vector<float> policy;   // 64
    float value;                 // scalar
};

class ReplayBuffer {
public:
    explicit ReplayBuffer(size_t capacity);

    void insert(const TrainSample& sample);
    void insert_batch(const std::vector<TrainSample>& batch);

    std::vector<TrainSample> sample(size_t batch_size) const;
    size_t size() const;

private:
    std::deque<TrainSample> buffer_;
    size_t capacity_;
    mutable std::mt19937 rng_;
};
