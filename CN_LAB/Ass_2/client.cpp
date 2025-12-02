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

    // Change this to server IP (in Mininet use: 10.0.0.1)
    inet_pton(AF_INET, "10.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    string fruit;
    int qty;

    cout << "Enter fruit name: ";
    cin >> fruit;

    cout << "Enter quantity: ";
    cin >> qty;

    string msg = fruit + " " + to_string(qty);
    send(sock, msg.c_str(), msg.size(), 0);

    char buffer[2048] = {0};
    read(sock, buffer, 2048);

    cout << "\n--- Server Response ---\n";
    cout << buffer << endl;

    close(sock);
    return 0;
}
