#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>
#include <random>
#include <filesystem>

// Simulate a dummy model with fixed size (e.g., 512 floats)
constexpr size_t NUM_WEIGHTS = 512;

int main() {
    spdlog::info("Starting training simulation...");

    // Generate dummy weights (random floats)
    std::vector<float> weights(NUM_WEIGHTS);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (auto& w : weights) {
        w = dist(rng);
    }

    // Create output directory if needed
    std::filesystem::create_directories("weights");

    // Save weights to binary file
    std::ofstream out("weights/weights.bin", std::ios::binary);
    if (!out) {
        spdlog::error("Failed to open weights/weights.bin for writing.");
        return 1;
    }

    out.write(reinterpret_cast<const char*>(weights.data()), weights.size() * sizeof(float));
    out.close();

    spdlog::info("Training complete. Weights written to weights/weights.bin");
    return 0;
}
