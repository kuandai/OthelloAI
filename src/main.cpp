#include <sstream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include "othello/board.hpp"
#include "othello/mcts.hpp"
#include "othello/greedy_evaluator.hpp"

void run_agent_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    // Set SO_REUSEADDR to reuse the port immediately after closing
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // Bind to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return;
    }
    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        return;
    }

    spdlog::info("Agent server listening on port {}...", port);

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    spdlog::info("Remote agent connected.");

    // Read side from remote agent
    char side_buf[16] = {};
    int side_bytes = read(client_fd, side_buf, sizeof(side_buf) - 1);
    std::string side_str(side_buf);
    Player my_side = (side_str.find("White") != std::string::npos) ? Player::WHITE : Player::BLACK;
    Player opp_side = othello::opponent(my_side);
    spdlog::info("Assigned side: {}", (my_side == Player::BLACK ? "Black" : "White"));

    // Init the board and MCTS evaluator
    OthelloBoard board;
    othello::GreedyEvaluator evaluator;
    othello::MCTS mcts(evaluator);

    char buffer[128];
    while (true) {
        std::memset(buffer, 0, sizeof(buffer));
        int bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes == 0) {
            spdlog::info("Client disconnected.");
            break;
        }

        int x, y, ms;
        std::istringstream input(buffer);
        if (!(input >> x >> y >> ms)) {
            spdlog::error("Malformed input received: {}", buffer);
            break;
        }
        spdlog::info("Received opponent move: {} {} ({} ms left)", x, y, ms); // Server does validation

        // Step 1: Apply opponent move
        Move opp_move = (x >= 0 && y >= 0) ? Move{x, y} : othello::PASS;
        board.apply_move(opp_side, opp_move);

        // Step 2: Apply opponent move to MCTS (preserve search tree)
        mcts.set_root(board, my_side);
        mcts.apply_move_to_root(opp_move);

        // Step 3: Check if we need to pass
        if (!board.has_valid_move(my_side)) {
            spdlog::info("No valid moves. Passing.");
            Move pass = othello::PASS;
            mcts.apply_move_to_root(pass);  // Let MCTS advance internally
            std::string response = "-1 -1\n";
            send(client_fd, response.c_str(), response.size(), 0);
            continue;
        }

        // Step 4: Run MCTS to find best move
        mcts.set_root(board, my_side);      // Set current board state
        mcts.run();                         // Run simulations
        Move bestMove = mcts.best_move();   // Get best move

        // Step 5: Apply and send our move
        board.apply_move(my_side, bestMove);
        mcts.apply_move_to_root(bestMove);  // Advance MCTS tree

        spdlog::info("Sending move: {} {}", bestMove.x, bestMove.y);

        std::ostringstream reply;
        reply << bestMove.x << " " << bestMove.y << "\n";
        std::string response = reply.str();
        send(client_fd, response.c_str(), response.size(), 0);
    }

    close(client_fd);
    close(server_fd);
    spdlog::info("Connection closed. Exiting.");
}

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[agent_server] [%^%l%$] %v");
    while (1) { run_agent_server(4000); }
    return 0;
}
