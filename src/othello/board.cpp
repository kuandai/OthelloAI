#include "othello/board.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>

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
    // Check for forced pass
    if (x == -1 && y == -1) {
        // Only valid if the player has no other legal moves
        return get_valid_moves(player).size() == 1 && get_valid_moves(player)[0] == othello::PASS;
    }

    if (!is_on_board(x, y)) return false;
    int idx = othello::to_index(x, y);
    if (((black_ | white_) >> idx) & 1) return false; // Space occupied

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
    // Forced pass
    if (moves.empty()) { moves.emplace_back(othello::PASS); }
    return moves;
}

bool OthelloBoard::apply_move(Player player, const Move& move) {
    if (move.x == -1 && move.y == -1) {
        current_player_ = othello::opponent(current_player_);
        return true;
    }
    if (!is_valid_move(player, move.x, move.y)) return false;
    int idx = othello::to_index(move.x, move.y);

    if (player == Player::BLACK)
        black_ |= (1ULL << idx);
    else
        white_ |= (1ULL << idx);

    flip_disks(player, move.x, move.y);
    current_player_ = othello::opponent(current_player_);
    return true;
}

OthelloBoard OthelloBoard::apply_move_copy(Player player, const Move& move) const {
    OthelloBoard next = *this;  // shallow copy is safe
    if (!next.apply_move(player, move))
        throw std::invalid_argument("Invalid move passed to apply_move_copy");
    return next;
}

int OthelloBoard::flip_disks(Player player, int x, int y) {
    // Create a bitmask for the current move position
    const uint64_t move_mask = 1ULL << othello::to_index(x, y);

    // This will accumulate all the opponent pieces we flip
    uint64_t flipped_mask = 0;

    // Choose the bitboards for the current player and opponent
    uint64_t self = (player == Player::BLACK) ? black_ : white_;
    uint64_t opp  = (player == Player::BLACK) ? white_ : black_;

    // Loop over all 8 directions
    for (int d = 0; d < 8; ++d) {
        int dx = DX[d];       // horizontal step
        int dy = DY[d];       // vertical step

        int cx = x + dx;      // current x position in this direction
        int cy = y + dy;      // current y position in this direction

        uint64_t captured = 0; // bitmask of opponent pieces in this direction

        // Step in the current direction until we hit the board edge
        while (is_on_board(cx, cy)) {
            int idx = othello::to_index(cx, cy);    // convert (x,y) to bit index
            uint64_t bit = 1ULL << idx;             // bitmask of that square

            if (opp & bit) {
                // Potential to flip
                captured |= bit;
            } else if (self & bit) {
                // Own piece after opponent - confirmed flip
                flipped_mask |= captured;
                break;
            } else {
                // Empty or no closing piece
                break;
            }

            // Step to the next square in the same direction
            cx += dx;
            cy += dy;
        }
    }

    // Update bitboards
    if (player == Player::BLACK) {
        black_ |= flipped_mask;          // Set flipped bits to black
        white_ &= ~flipped_mask;         // Clear those bits from white
    } else {
        white_ |= flipped_mask;
        black_ &= ~flipped_mask;
    }

    // Return the number of pieces flipped (for debugging or scoring)
    return __builtin_popcountll(flipped_mask);
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
    // channel 2: valid move mask
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            if (is_valid_move(current_player, x, y)) {
                int idx = y * 8 + x;
                tensor[128 + idx] = 1.0f;
            }
        }
    }

    return tensor;
}

Player OthelloBoard::current_player() const {
    return current_player_;
}