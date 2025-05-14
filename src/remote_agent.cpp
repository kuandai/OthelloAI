#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <array>
#include <memory>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

enum Side { BLACK, WHITE };

const char REMOTE_ADDR[] = "127.0.0.1";
const int  REMOTE_PORT = 4000;
#ifdef _WIN32
constexpr bool WIN32_BUILD = true;
#else
constexpr bool WIN32_BUILD = false;
#endif

int connectToAgent(const std::string& hostname, int port) {
    #ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        exit(1);
    }
    #endif

    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent* server = gethostbyname(hostname.c_str());

    if (!server) {
        std::cerr << "ERROR: No such host\n";
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    std::memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    std::memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    return sockfd;
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    // Retrieve WSL Address
    std::array<char, 128> buffer;
    std::string wsl_addr;
    try {

        // Use a pipe to run the shell command
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("wsl hostname -I", "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("Failed to run wsl hostname -I");
        }

        // Read the command output
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            wsl_addr += buffer.data();
        }

        // Remove any trailing newline
        wsl_addr.erase(wsl_addr.find_last_not_of(" \n\r\t") + 1);
    } catch (const std::exception& e) {
        std::cerr << "Unable to retrieve WSL address: " << wsl_addr << std::endl;
    }
    #endif
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " side" << std::endl;
        return 1;
    }

    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--version") {
            std::cerr << "remote_agent compiled for " << (WIN32_BUILD ? "WIN32" : "Unix") << std::endl;
	    return 0;
	}
    }

    Side side = (!strcmp(argv[1], "Black")) ? BLACK : WHITE;
    const std::string color = (side == BLACK) ? "Black" : "White";

    // Connect to remote RL agent
    #ifdef _WIN32
    int sockfd = connectToAgent(wsl_addr, REMOTE_PORT);
    #endif
    #ifndef _WIN32
    int sockfd = connectToAgent(std::string(REMOTE_ADDR), REMOTE_PORT);
    #endif

    // Send the side once after connecting
    std::string side_msg = color + "\n";
    if (send(sockfd, side_msg.c_str(), side_msg.size(), 0) < 0) {
        perror("ERROR sending side to agent");
        return 1;
    }

    // Init done signal to Java wrapper
    std::cout << "Init done" << std::endl;
    std::cout.flush();

    // Main game loop
    int moveX, moveY, msLeft;
    while (std::cin >> moveX >> moveY >> msLeft) {
        std::ostringstream msg;
        msg << moveX << " " << moveY << " " << msLeft << std::endl;;
        std::string msgStr = msg.str();

        // Send to remote agent
        if (send(sockfd, msgStr.c_str(), msgStr.size(), 0) < 0) {
            perror("ERROR writing to socket");
            break;
        }

        // Read response
        char buffer[256] = {0};
        int n = recv(sockfd, buffer, 255, 0);
        if (n <= 0) {
            std::cerr << "ERROR reading from socket or remote disconnected" << std::endl;
            break;
        }

        int x, y;
        std::istringstream respStream((std::string(buffer))); // Double parenthesis to mitigate aggressive intellisense; C++ spec is vague
        respStream >> x >> y;

        std::cout << x << " " << y << std::endl;
        std::cout.flush();
    }

    #ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
    #else
    close(sockfd);
    #endif

    return 0;
}
