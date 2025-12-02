#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    socklen_t serv_len = sizeof(serv_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    // CHANGE THIS TO SERVER IP (Mininet: 10.0.0.1)
    inet_pton(AF_INET, "10.0.0.1", &serv_addr.sin_addr);

    string fruit;
    int qty;

    cout << "Enter fruit name: ";
    cin >> fruit;

    cout << "Enter quantity: ";
    cin >> qty;

    string msg = fruit + " " + to_string(qty);

    // Send request
    sendto(sock, msg.c_str(), msg.size(), 0,
           (struct sockaddr*)&serv_addr, serv_len);

    // Receive response
    char buffer[2048] = {0};
    recvfrom(sock, buffer, 2048, 0, 
             (struct sockaddr*)&serv_addr, &serv_len);

    cout << "\n--- SERVER RESPONSE ---\n";
    cout << buffer << endl;

    close(sock);
    return 0;
}
