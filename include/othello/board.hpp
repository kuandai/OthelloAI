#pragma once
#include <cstdint>
#include <vector>
#include <optional>

enum class Player { NONE = 0, BLACK = 1, WHITE = 2 };

struct Move {
    int x, y;
    Move(int x, int y) : x(x), y(y) {}
};
// Operator overload for comparisons
inline bool operator==(const Move& a, const Move& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Move& a, const Move& b) {
    return !(a == b);
}

namespace othello {
    // Sentinel for forced pass
    const inline Move PASS(-1, -1);

    inline int to_index(int x, int y) {
        return y * 8 + x;
    }

    inline Player opponent(Player player) {
        return player == Player::BLACK ? Player::WHITE : Player::BLACK;
    }
} // namespace othello

class OthelloBoard {
public:
    OthelloBoard();

    bool is_valid_move(Player player, int x, int y) const;
    std::vector<Move> get_valid_moves(Player player) const;
    bool apply_move(Player player, const Move& move);
    OthelloBoard apply_move_copy(Player player, const Move& move) const;
    bool has_valid_move(Player player) const;
    bool is_game_over() const;
    int count_disks(Player player) const;
    Player get_winner() const;
    Player current_player() const;

    Player at(int x, int y) const;
    std::vector<float> to_tensor(Player current_player) const;

    // Getters for unit tests
    public:
        uint64_t debug_black() const { return black_; }
        uint64_t debug_white() const { return white_; }

private:
    uint64_t black_;  // bitboard
    uint64_t white_;  // bitboard
    Player current_player_;

    bool is_on_board(int x, int y) const;
    int flip_disks(Player player, int x, int y);
};
