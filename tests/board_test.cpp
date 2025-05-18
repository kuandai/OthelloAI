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

TEST(OthelloBoardTest, RejectsIllegalMove) {
    OthelloBoard board;
    EXPECT_FALSE(board.apply_move(Player::BLACK, Move(0, 0)));
}

TEST(OthelloBoardTest, AllowsPassWhenNoValidMoves) {
    OthelloBoard board;

    // Fill board except for one edge and give all to BLACK
    board = OthelloBoard();
    for (int i = 0; i < 64; ++i) {
        if (i != othello::to_index(7, 7))
            board.apply_move(Player::BLACK, Move(i % 8, i / 8));
    }

    auto white_moves = board.get_valid_moves(Player::WHITE);
    EXPECT_EQ(white_moves.size(), 1);
    EXPECT_EQ(white_moves[0], othello::PASS);
}

TEST(OthelloBoardTest, WinnerDetermination) {
    OthelloBoard board;

    // Simulate a board with more black pieces
    for (int i = 0; i < 64; ++i) {
        if (i < 40)
            board.apply_move(Player::BLACK, Move(i % 8, i / 8));
        else
            board.apply_move(Player::WHITE, Move(i % 8, i / 8));
    }

    EXPECT_EQ(board.get_winner(), Player::BLACK);
}

TEST(OthelloBoardTest, BitboardsDoNotOverlap) {
    OthelloBoard board;
    board.apply_move(Player::BLACK, Move(2, 3));
    board.apply_move(Player::WHITE, Move(2, 2));
    EXPECT_EQ(board.debug_black() & board.debug_white(), 0ULL);  // Ensure no overlapping bits
}