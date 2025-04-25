#include "replay/buffer.hpp"
#include <algorithm>
#include <random>
#include <cassert>

ReplayBuffer::ReplayBuffer(size_t capacity)
    : capacity_(capacity), rng_(std::random_device{}()) {}

void ReplayBuffer::insert(const TrainSample& sample) {
    if (buffer_.size() >= capacity_) {
        buffer_.pop_front(); // Remove the oldest sample
    }
    buffer_.push_back(sample);
}

void ReplayBuffer::insert_batch(const std::vector<TrainSample>& batch) {
    for (const auto& sample : batch) {
        insert(sample);
    }
}

std::vector<TrainSample> ReplayBuffer::sample(size_t batch_size) const {
    assert(buffer_.size() >= batch_size);

    std::vector<TrainSample> batch;
    batch.reserve(batch_size);

    std::uniform_int_distribution<size_t> dist(0, buffer_.size() - 1);

    for (size_t i = 0; i < batch_size; ++i) {
        size_t idx = dist(rng_);
        batch.push_back(buffer_[idx]);
    }

    return batch;
}

size_t ReplayBuffer::size() const {
    return buffer_.size();
}