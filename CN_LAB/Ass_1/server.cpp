#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(5000);

    // Bind the socket
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    // Listen for connections
    listen(server_fd, 1);
    cout << "Server waiting for connection..." << endl;

    // Accept the connection
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    cout << "Client connected!" << endl;

    // Read client message
    char buffer[1024] = {0};
    read(new_socket, buffer, 1024);
    cout << "Client says: " << buffer << endl;

    // Send response
    string message = "Hello";
    send(new_socket, message.c_str(), message.size(), 0);

    close(new_socket);
    close(server_fd);
    return 0;
}
