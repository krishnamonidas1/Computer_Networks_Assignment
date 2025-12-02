#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <set>
#include <ctime>
#include <sstream>
#include <cstring>

using namespace std;

string getTimestamp() {
    time_t now = time(0);
    char* dt = ctime(&now);
    dt[strlen(dt)-1] = '\0';
    return string(dt);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Fruit stock
    unordered_map<string, pair<int,string>> fruits = {
        {"apple", {50, "Never"}},
        {"banana", {20, "Never"}},
        {"mango", {30, "Never"}}
    };

    // Store customer identifiers
    set<string> unique_customers;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5000);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    cout << "SERVER RUNNING..." << endl;

    // SERVER LOOP
    while (true) {
        cout << "\nWaiting for client..." << endl;
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        
        string customer_id = string(inet_ntoa(address.sin_addr)) + ":" + to_string(ntohs(address.sin_port));
        unique_customers.insert(customer_id);

        cout << "Connected → " << customer_id << endl;

        // Receive message
        char buffer[1024] = {0};
        read(client_socket, buffer, 1024);

        stringstream ss(buffer);
        string fname;
        int qty;
        ss >> fname >> qty;

        string reply;

        // Check fruit exists
        if (fruits.find(fname) == fruits.end()) {
            reply = "Fruit not found!";
        }
        else {
            int available = fruits[fname].first;

            if (qty > available) {
                reply = "Regret! Only " + to_string(available) + " " + fname + " left.\n";
            } else {
                // Update stock
                fruits[fname].first -= qty;
                fruits[fname].second = getTimestamp();

                reply = "Purchase successful!\n";
                reply += "Remaining " + fname + " = " + to_string(fruits[fname].first) + "\n";
                reply += "Last sold time = " + fruits[fname].second + "\n";
            }
        }

        reply += "Unique customers so far = " + to_string(unique_customers.size()) + "\n";

        send(client_socket, reply.c_str(), reply.length(), 0);

        close(client_socket);
    }

    close(server_fd);
    return 0;
}
