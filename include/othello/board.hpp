#pragma once
#include <vector>
#include <optional>

enum class Player {
    NONE,
    BLACK,
    WHITE
};

inline Player opponent(Player p) {
    return (p == Player::BLACK) ? Player::WHITE : Player::BLACK;
}

struct Move {
    int x, y;
    Move(int x, int y) : x(x), y(y) {}
};

class OthelloBoard {
public:
    OthelloBoard();

    Player at(int x, int y) const;
    bool is_on_board(int x, int y) const;

    std::vector<Move> get_valid_moves(Player player) const;
    bool apply_move(Player player, const Move& move); // returns false if invalid
    bool has_valid_move(Player player) const;

    bool is_game_over() const;
    int count_disks(Player player) const;

private:
    Player board[8][8];

    bool is_valid_move(Player player, int x, int y) const;
    int flip_disks(Player player, int x, int y); // returns number of flipped disks
};
