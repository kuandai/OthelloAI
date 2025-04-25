#pragma once
#include <cstdint>
#include <vector>
#include <optional>

enum class Player { NONE = 0, BLACK = 1, WHITE = 2 };

struct Move {
    int x, y;
    Move(int x, int y) : x(x), y(y) {}
};

namespace othello {
    inline int to_index(int x, int y);
    inline Player opponent(Player player);
} // namespace othello

class OthelloBoard {
public:
    OthelloBoard();

    bool is_valid_move(Player player, int x, int y) const;
    std::vector<Move> get_valid_moves(Player player) const;
    bool apply_move(Player player, const Move& move);
    bool has_valid_move(Player player) const;
    bool is_game_over() const;
    int count_disks(Player player) const;
    Player get_winner() const;

    Player at(int x, int y) const;
    std::vector<float> to_tensor(Player current_player) const;

private:
    uint64_t black_;  // bitboard
    uint64_t white_;  // bitboard

    bool is_on_board(int x, int y) const;
    int flip_disks(Player player, int x, int y);
};
