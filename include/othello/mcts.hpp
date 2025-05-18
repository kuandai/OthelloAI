#pragma once

#include "othello/board.hpp"
#include "othello/mctsnode.hpp"
#include "othello/evaluator.hpp"

#include <memory>
#include <random>

namespace othello {

class MCTS {
public:
    MCTS(Evaluator& evaluator, int num_simulations = 800, float c_puct = 1.5f);
    
    // Run full MCTS from current root
    void run();

    // Choose the best move after search
    Move best_move(bool temperature = false);

    // Advance root node after external move is made
    void apply_move_to_root(const Move& move);

    // Reset root to a new game state
    void set_root(const OthelloBoard& board, Player player);

    // For training output
    std::vector<float> get_policy_target() const;
    float get_value_target() const;

private:
    // Tree structure
    std::unique_ptr<MCTSNode> root_;
    Evaluator& evaluator_;

    // MCTS parameters
    int num_simulations_;
    float c_puct_;

    // Random generator (for Dirichlet noise, temperature sampling)
    std::mt19937 rng_;

    // Internal search steps
    void run_simulation();
    MCTSNode* select_leaf(MCTSNode* node);
    void backpropagate(MCTSNode* node, float value);

    // Helper for adding Dirichlet noise at root
    void add_dirichlet_noise(MCTSNode* node);
};

}  // namespace othello
