#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    // Server IP → find using: ifconfig
    inet_pton(AF_INET, "YOUR_SERVER_IP", &serv_addr.sin_addr);

    // Connect to server
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // Send "Hi"
    string msg = "Hi";
    send(sock, msg.c_str(), msg.size(), 0);

    // Receive response
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    cout << "Server says: " << buffer << endl;

    close(sock);
    return 0;
}
