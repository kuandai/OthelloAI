#include "othello/board.hpp"
#include <algorithm>

namespace {
    const int DX[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
    const int DY[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
}

OthelloBoard::OthelloBoard() {
    for (auto& row : board)
        std::fill(std::begin(row), std::end(row), Player::NONE);

    board[3][3] = board[4][4] = Player::WHITE;
    board[3][4] = board[4][3] = Player::BLACK;
}

Player OthelloBoard::at(int x, int y) const {
    return is_on_board(x, y) ? board[y][x] : Player::NONE;
}

bool OthelloBoard::is_on_board(int x, int y) const {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

bool OthelloBoard::is_valid_move(Player player, int x, int y) const {
    if (!is_on_board(x, y) || board[y][x] != Player::NONE) return false;

    Player opp = opponent(player);
    for (int d = 0; d < 8; ++d) {
        int nx = x + DX[d], ny = y + DY[d];
        bool found_opp = false;
        while (is_on_board(nx, ny) && board[ny][nx] == opp) {
            nx += DX[d];
            ny += DY[d];
            found_opp = true;
        }
        if (found_opp && is_on_board(nx, ny) && board[ny][nx] == player)
            return true;
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

    board[move.y][move.x] = player;
    flip_disks(player, move.x, move.y);
    return true;
}

int OthelloBoard::flip_disks(Player player, int x, int y) {
    int flipped = 0;
    Player opp = opponent(player);

    for (int d = 0; d < 8; ++d) {
        int nx = x + DX[d], ny = y + DY[d];
        std::vector<std::pair<int, int>> path;

        while (is_on_board(nx, ny) && board[ny][nx] == opp) {
            path.emplace_back(nx, ny);
            nx += DX[d];
            ny += DY[d];
        }

        if (!path.empty() && is_on_board(nx, ny) && board[ny][nx] == player) {
            for (auto [fx, fy] : path) {
                board[fy][fx] = player;
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
    int count = 0;
    for (const auto& row : board)
        for (const auto& cell : row)
            if (cell == player) ++count;
    return count;
}
