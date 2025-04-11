#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

enum Side { BLACK, WHITE };

static char REMOTE_ADDR[] = "127.0.0.1";
static int  REMOTE_PORT = 4000;

int connectToAgent(const std::string& hostname, int port) {
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
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " side" << std::endl;
        return 1;
    }

    Side side = (!strcmp(argv[1], "Black")) ? BLACK : WHITE;
    const std::string color = (side == BLACK) ? "Black" : "White";

    // Connect to remote RL agent
    int sockfd = connectToAgent(std::string(REMOTE_ADDR), REMOTE_PORT); // adjust port as needed

    std::cout << "Init done" << std::endl;
    std::cout.flush();

    int moveX, moveY, msLeft;
    while (std::cin >> moveX >> moveY >> msLeft) {
        std::ostringstream msg;
        msg << moveX << " " << moveY << " " << msLeft << " " << color << "\n";
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

        std::string response(buffer);
        std::istringstream respStream(response);
        int x, y;
        respStream >> x >> y;
        std::cout << x << " " << y << std::endl;
        std::cout.flush();
    }

    close(sockfd);
    return 0;
}
