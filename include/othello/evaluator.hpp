#pragma once

#include "othello/board.hpp"
#include <vector>
#include <utility>

namespace othello {

class Evaluator {
public:
    virtual ~Evaluator() = default;

    // Evaluate a board state and return (policy vector, value)
    // - policy: vector of size 64 for each board position
    // - value: scalar in [-1, 1] from current_player's perspective
    virtual std::pair<std::vector<float>, float> evaluate(const OthelloBoard& board, Player current_player) = 0;
};

}  // namespace othello