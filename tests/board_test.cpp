#include <gtest/gtest.h>
#include "othello/board.hpp"

TEST(OthelloBoardTest, InitialStateHasFourPieces) {
    OthelloBoard board;
    EXPECT_EQ(board.count_disks(Player::BLACK), 2);
    EXPECT_EQ(board.count_disks(Player::WHITE), 2);
    EXPECT_FALSE(board.is_game_over());
}

TEST(OthelloBoardTest, ValidMovesForBlackAtStart) {
    OthelloBoard board;
    auto moves = board.get_valid_moves(Player::BLACK);
    EXPECT_EQ(moves.size(), 4);
}

TEST(OthelloBoardTest, ApplyValidMoveAndFlip) {
    OthelloBoard board;
    auto moves = board.get_valid_moves(Player::BLACK);
    ASSERT_FALSE(moves.empty());
    auto move = moves[0];

    bool applied = board.apply_move(Player::BLACK, move);
    EXPECT_TRUE(applied);
    EXPECT_GT(board.count_disks(Player::BLACK), 2);
}
