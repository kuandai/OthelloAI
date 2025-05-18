#pragma once
#include "othello/board.hpp"
#include <unordered_map>
#include <memory>
#include <cmath>
#include <functional>
#include <cassert>
#include <spdlog/spdlog.h>

namespace othello {

constexpr int PASS_INDEX = 64;
static constexpr int kNumMoves = 65; // 64 board positions + pass

struct MCTSNode {
    OthelloBoard board;
    Player current_player;

    std::vector<Move> legal_moves;
    uint64_t legal_move_mask = 0;
    std::unordered_map<int, std::unique_ptr<MCTSNode>> children;

    // Move statistics
    std::vector<float> prior;        // P(s,a)
    std::vector<float> value_sum;    // Q sum
    std::vector<int> visit_count;    // N(s,a)

    bool is_expanded = false;
    MCTSNode* parent = nullptr;
    Move move_from_parent = othello::PASS;

    MCTSNode(const OthelloBoard& board, Player player, MCTSNode* parent = nullptr, Move move = othello::PASS)
        : board(board), current_player(player), parent(parent), move_from_parent(move),
          prior(kNumMoves, 0.0f), value_sum(kNumMoves, 0.0f), visit_count(kNumMoves, 0) {}

    float get_mean_value(int idx) const {
        int n = visit_count[idx];
        return n == 0 ? 0.0f : value_sum[idx] / n;
    }

    float ucb_score(int idx, float total_visits, float c_puct) const {
        float q = get_mean_value(idx);
        float p = prior[idx];
        int n = visit_count[idx];
        return q + c_puct * p * std::sqrt(total_visits) / (1 + n);
    }

    bool is_terminal() const {
        return board.is_game_over();
    }

    void expand(std::function<std::pair<std::vector<float>, float>(const OthelloBoard&, Player)> evaluator) {
        if (is_expanded) return;

        // Step 1: Evaluate with NN
        auto [policy, value] = evaluator(board, current_player);
        assert(policy.size() == 65);

        // Step 2: Get legal moves
        legal_moves = board.get_valid_moves(current_player);
        legal_move_mask = 0;
        spdlog::debug("Expanding node for player {}", (current_player == Player::BLACK ? "BLACK" : "WHITE"));
        spdlog::debug("Legal moves: {}", legal_moves.size());

        float policy_sum = 0.0f;

        for (const auto& move : legal_moves) {
            if (move == othello::PASS) continue;
            int idx = othello::to_index(move.x, move.y);
            float p = policy[idx];

            prior[idx] = p;
            policy_sum += p;
            legal_move_mask |= (1ULL << idx);
        }

        // Step 3: Normalize priors
        if (policy_sum > 1e-8f) {
            for (int i = 0; i < 64; ++i)
                prior[i] /= policy_sum;
        }

        is_expanded = true;
    }
};

} // namespace othello