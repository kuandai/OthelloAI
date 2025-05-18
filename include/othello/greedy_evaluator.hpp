#pragma once

#include "othello/evaluator.hpp"

namespace othello {

class GreedyEvaluator : public Evaluator {
public:
    std::pair<std::vector<float>, float> evaluate(const OthelloBoard& board, Player current_player) override {
        // Step 1: create uniform policy over legal moves
        std::vector<float> policy(65, 0.0f);
        auto moves = board.get_valid_moves(current_player);
        float p = 1.0f / static_cast<float>(moves.size());

        for (const auto& move : moves) {
            int idx = (move == othello::PASS) ? 64 : othello::to_index(move.x, move.y);
            policy[idx] = p;
        }

        // Step 2: compute value = normalized disk differential
        int black = board.count_disks(Player::BLACK);
        int white = board.count_disks(Player::WHITE);

        float value = 0.0f;
        if (black + white > 0) {
            if (current_player == Player::WHITE)
                value = static_cast<float>(black - white) / (black + white);
            else
                value = static_cast<float>(white - black) / (black + white);
        }

        return {policy, value};
    }
};

} // namespace othello
