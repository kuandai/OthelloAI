#include "othello/board.hpp"
#include <vector>
#include <algorithm>

namespace {
    const int DX[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
    const int DY[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
} // Anonymous namespace

OthelloBoard::OthelloBoard() {
    black_ = (1ULL << othello::to_index(4, 3)) | (1ULL << othello::to_index(3, 4));
    white_ = (1ULL << othello::to_index(3, 3)) | (1ULL << othello::to_index(4, 4));
}

bool OthelloBoard::is_on_board(int x, int y) const {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

Player OthelloBoard::at(int x, int y) const {
    if (!is_on_board(x, y)) return Player::NONE;
    int idx = othello::to_index(x, y);
    if ((black_ >> idx) & 1) return Player::BLACK;
    if ((white_ >> idx) & 1) return Player::WHITE;
    return Player::NONE;
}

bool OthelloBoard::is_valid_move(Player player, int x, int y) const {
    if (!is_on_board(x, y)) return false;
    int idx = othello::to_index(x, y);
    if (((black_ | white_) >> idx) & 1) return false;

    Player opp = othello::opponent(player);
    uint64_t self = (player == Player::BLACK) ? black_ : white_;
    uint64_t other = (player == Player::BLACK) ? white_ : black_;

    for (int d = 0; d < 8; ++d) {
        int nx = x + DX[d], ny = y + DY[d];
        int count = 0;
        while (is_on_board(nx, ny) && at(nx, ny) == opp) {
            nx += DX[d];
            ny += DY[d];
            ++count;
        }
        if (count > 0 && is_on_board(nx, ny) && at(nx, ny) == player) return true;
    }
    return false;
}

std::vector<Move> OthelloBoard::get_valid_moves(Player player) const {
    std::vector<Move> moves;
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if (is_valid_move(player, x, y))
                moves.emplace_back(x, y);
    return moves;
}

bool OthelloBoard::apply_move(Player player, const Move& move) {
    if (!is_valid_move(player, move.x, move.y)) return false;
    int idx = othello::to_index(move.x, move.y);

    if (player == Player::BLACK)
        black_ |= (1ULL << idx);
    else
        white_ |= (1ULL << idx);

    flip_disks(player, move.x, move.y);
    return true;
}

int OthelloBoard::flip_disks(Player player, int x, int y) {
    int flipped = 0;
    Player opp = othello::opponent(player);

    for (int d = 0; d < 8; ++d) {
        int nx = x + DX[d], ny = y + DY[d];
        std::vector<int> path;

        while (is_on_board(nx, ny) && at(nx, ny) == opp) {
            path.push_back(othello::to_index(nx, ny));
            nx += DX[d];
            ny += DY[d];
        }

        if (!path.empty() && is_on_board(nx, ny) && at(nx, ny) == player) {
            for (int idx : path) {
                if (player == Player::BLACK) {
                    black_ |= (1ULL << idx);
                    white_ &= ~(1ULL << idx);
                } else {
                    white_ |= (1ULL << idx);
                    black_ &= ~(1ULL << idx);
                }
                ++flipped;
            }
        }
    }

    return flipped;
}

bool OthelloBoard::has_valid_move(Player player) const {
    return !get_valid_moves(player).empty();
}

bool OthelloBoard::is_game_over() const {
    return !has_valid_move(Player::BLACK) && !has_valid_move(Player::WHITE);
}

int OthelloBoard::count_disks(Player player) const {
    uint64_t board = (player == Player::BLACK) ? black_ : white_;
    return __builtin_popcountll(board);
}

Player OthelloBoard::get_winner() const {
    int black = count_disks(Player::BLACK);
    int white = count_disks(Player::WHITE);
    if (black > white) return Player::BLACK;
    if (white > black) return Player::WHITE;
    return Player::NONE;
}

std::vector<float> OthelloBoard::to_tensor(Player current_player) const {
    std::vector<float> tensor(8 * 8 * 3, 0.0f);
    uint64_t cur = (current_player == Player::BLACK) ? black_ : white_;
    uint64_t opp = (current_player == Player::BLACK) ? white_ : black_;

    for (int i = 0; i < 64; ++i) {
        if ((cur >> i) & 1) tensor[i] = 1.0f;             // channel 0
        if ((opp >> i) & 1) tensor[64 + i] = 1.0f;        // channel 1
    }

    float turn_val = (current_player == Player::BLACK) ? 1.0f : 0.0f;
    std::fill(tensor.begin() + 128, tensor.end(), turn_val);  // channel 2

    return tensor;
}
