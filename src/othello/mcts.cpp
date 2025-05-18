#include "othello/mcts.hpp"
#include <ctime>
#include <stdexcept>

namespace othello {

MCTS::MCTS(Evaluator& evaluator, int num_simulations, float c_puct)
    : evaluator_(evaluator),
      num_simulations_(num_simulations),
      c_puct_(c_puct)
{
    // Seed RNG with current time
    rng_.seed(time(nullptr));
}

void MCTS::run() {
    using namespace std::chrono;

    int max_depth = 0;
    long long total_depth = 0;
    int terminal_count = 0;

    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_simulations_; ++i) {
        int depth = 0;
        MCTSNode* node = root_.get();

        // Estimate depth before expansion
        while (node->is_expanded && !node->is_terminal()) {
            int total_visits = std::accumulate(node->visit_count.begin(), node->visit_count.end(), 0);
            float best_score = -1e9;
            int best_move_idx = -1;

            for (int j = 0; j < 64; ++j) {
                if ((node->legal_move_mask >> j) & 1) {
                    float score = node->ucb_score(j, total_visits, c_puct_);
                    if (score > best_score) {
                        best_score = score;
                        best_move_idx = j;
                    }
                }
            }

            if (node->visit_count.size() == 65) {
                float score = node->ucb_score(64, total_visits, c_puct_);
                if (score > best_score) {
                    best_score = score;
                    best_move_idx = 64;
                }
            }

            if (best_move_idx == -1 || node->children.find(best_move_idx) == node->children.end())
                break;

            node = node->children[best_move_idx].get();
            depth++;
        }

        if (node->is_terminal()) {
            terminal_count++;
        }

        max_depth = std::max(max_depth, depth);
        total_depth += depth;

        run_simulation();  // Do the actual MCTS simulation
    }

    auto end = high_resolution_clock::now();
    duration<double> elapsed = end - start;

    spdlog::info("[MCTS Stats] Max depth: {}, Avg depth: {:.2f}, Terminal leaves: {}, Time: {:.3f}s",
                 max_depth,
                 static_cast<float>(total_depth) / num_simulations_,
                 terminal_count,
                 elapsed.count());
}


Move MCTS::best_move(bool temperature) {
    assert(root_ && "Must call set_root() and run() before best_move()");

    const auto& visits = root_->visit_count;
    const auto& moves = root_->legal_moves;

    if (!temperature) {
        // Deterministic: choose move with max visits
        int best_idx = -1;
        int max_visits = -1;
        for (int i = 0; i < visits.size(); ++i) {
            if (visits[i] > max_visits) {
                max_visits = visits[i];
                best_idx = i;
            }
        }
        return (best_idx == 64) ? othello::PASS : Move(best_idx % 8, best_idx / 8);
    } else {
        // Stochastic: sample from visit count distribution
        std::discrete_distribution<int> dist(visits.begin(), visits.end());
        int sampled_idx = dist(rng_);
        return (sampled_idx == 64) ? othello::PASS : Move(sampled_idx % 8, sampled_idx / 8);
    }
}

void MCTS::apply_move_to_root(const Move& move) {
    assert(root_ && "Must call set_root() before apply_move_to_root()");

    int move_idx = (move == othello::PASS) ? 64 : othello::to_index(move.x, move.y);

    auto it = root_->children.find(move_idx);

    if (it != root_->children.end()) {
        // Reuse existing subtree under this move
        root_ = std::move(it->second);
        root_->parent = nullptr;  // Detach from previous parent
    } else {
        // Reconstruct board by applying move manually
        OthelloBoard next_board = root_->board;
        next_board.apply_move(root_->current_player, move);
        Player next_player = othello::opponent(root_->current_player);
        root_ = std::make_unique<MCTSNode>(next_board, next_player);
    }
}

void MCTS::set_root(const OthelloBoard& board, Player player) {
    root_ = std::make_unique<MCTSNode>(board, player);
}

std::vector<float> MCTS::get_policy_target() const {
    assert(root_);

    std::vector<float> policy(root_->visit_count.size(), 0.0f);
    float sum = std::accumulate(root_->visit_count.begin(), root_->visit_count.end(), 0.0f);

    if (sum > 0.0f) {
        for (size_t i = 0; i < policy.size(); ++i) {
            policy[i] = static_cast<float>(root_->visit_count[i]) / sum;
        }
    }

    return policy;
}

float MCTS::get_value_target() const {
    assert(root_);

    if (!root_->is_terminal()) {
        throw std::runtime_error("Value target requested for non-terminal node");
    }

    int black = root_->board.count_disks(Player::BLACK);
    int white = root_->board.count_disks(Player::WHITE);

    if (black == white) return 0.0f;

    bool current_is_black = root_->current_player == Player::BLACK;
    bool current_wins = (current_is_black && black > white) || (!current_is_black && white > black);

    return current_wins ? 1.0f : -1.0f;
}


void MCTS::run_simulation() {
    // 1. Selection: follow UCB until a leaf node
    MCTSNode* node = select_leaf(root_.get());

    // 2. Check if terminal node
    if (node->is_terminal()) {
        // Terminal states return final game outcome
        int black = node->board.count_disks(Player::BLACK);
        int white = node->board.count_disks(Player::WHITE);
        float value = (node->current_player == Player::BLACK)
                        ? static_cast<float>(black - white) / 64.0f
                        : static_cast<float>(white - black) / 64.0f;
        backpropagate(node, value);
        return;
    }

    // 3. Expansion and evaluation
    node->expand([&](const OthelloBoard& board, Player player) {
        return evaluator_.evaluate(board, player);
    });

    // 4. Re-evaluate the leaf after expansion
    // This will be the value from the current player's perspective
    auto [_, value] = evaluator_.evaluate(node->board, node->current_player);

    // 5. Backpropagate value up the tree
    backpropagate(node, value);
}

MCTSNode* MCTS::select_leaf(MCTSNode* node) {
    while (node->is_expanded && !node->is_terminal()) {
        int total_visits = 0;
        for (int n : node->visit_count)
            total_visits += n;

        float best_score = -1e9;
        int best_move_idx = -1;

        for (int i = 0; i < 64; ++i) {
            if ((node->legal_move_mask >> i) & 1) {
                float score = node->ucb_score(i, total_visits, c_puct_);
                if (score > best_score) {
                    best_score = score;
                    best_move_idx = i;
                }
            }
        }

        // Handle PASS separately
        if (node->visit_count.size() == 65) {
            float score = node->ucb_score(64, total_visits, c_puct_);
            if (score > best_score) {
                best_score = score;
                best_move_idx = 64;
            }
        }

        // Defensive fallback
        if (best_move_idx == -1) return node;

        // Create or follow child node
        auto it = node->children.find(best_move_idx);
        if (it == node->children.end()) {
            Move move = (best_move_idx == 64) ? othello::PASS :
                         Move(best_move_idx % 8, best_move_idx / 8);
            OthelloBoard next_board = node->board;
            next_board.apply_move(node->current_player, move);
            Player next_player = othello::opponent(node->current_player);

            auto child = std::make_unique<MCTSNode>(next_board, next_player, node, move);
            node->children[best_move_idx] = std::move(child);
            return node->children[best_move_idx].get();
        } else {
            node = it->second.get();
        }
    }

    return node;
}

void MCTS::backpropagate(MCTSNode* node, float value) {
    while (node->parent != nullptr) {
        MCTSNode* parent = node->parent;
        int move_idx;

        if (node->move_from_parent == othello::PASS) {
            move_idx = 64;  // PASS index
        } else {
            move_idx = othello::to_index(node->move_from_parent.x, node->move_from_parent.y);
        }

        parent->visit_count[move_idx] += 1;
        parent->value_sum[move_idx] += value;

        // Flip value for the parent’s perspective
        value = -value;

        node = parent;
    }
}

void MCTS::add_dirichlet_noise(MCTSNode* node) {
    assert(node && node == root_.get());

    const float epsilon = 0.25f;
    const float alpha = 0.3f;

    // Step 1: collect indices of legal (non-pass) moves
    std::vector<int> legal_idxs;
    for (int i = 0; i < 64; ++i) {
        if ((node->legal_move_mask >> i) & 1)
            legal_idxs.push_back(i);
    }

    if (legal_idxs.empty()) return;

    // Step 2: sample Dirichlet noise
    std::gamma_distribution<float> gamma(alpha, 1.0f);
    std::vector<float> dirichlet(legal_idxs.size());
    float sum = 0.0f;

    for (float& val : dirichlet) {
        val = gamma(rng_);
        sum += val;
    }
    for (float& val : dirichlet)
        val /= sum;

    // Step 3: mix noise into prior
    for (size_t i = 0; i < legal_idxs.size(); ++i) {
        int idx = legal_idxs[i];
        node->prior[idx] = (1 - epsilon) * node->prior[idx] + epsilon * dirichlet[i];
    }
}

} // namespace othello