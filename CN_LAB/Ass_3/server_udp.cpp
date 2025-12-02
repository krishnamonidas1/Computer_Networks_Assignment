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
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Fruit stock
    unordered_map<string, pair<int,string>> fruits = {
        {"apple", {50, "Never"}},
        {"banana", {20, "Never"}},
        {"mango", {30, "Never"}}
    };

    // Unique customer <IP:PORT> storage
    set<string> unique_customers;

    // Create UDP socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5000);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    cout << "UDP SERVER RUNNING..." << endl;

    while (true) {
        char buffer[1024] = {0};

        // Receive from client
        recvfrom(server_fd, buffer, 1024, 0, 
                 (struct sockaddr*)&client_addr, &client_len);

        string customer_id = string(inet_ntoa(client_addr.sin_addr)) + 
                             ":" + to_string(ntohs(client_addr.sin_port));

        unique_customers.insert(customer_id);

        cout << "Received request from " << customer_id << endl;

        // Parse client message
        stringstream ss(buffer);
        string fname;
        int qty;
        ss >> fname >> qty;

        string reply;

        if (fruits.find(fname) == fruits.end()) {
            reply = "Fruit not found!";
        } else {
            int available = fruits[fname].first;

            if (qty > available) {
                reply = "Regret! Only " + to_string(available) + " " + fname + " left.\n";
            } else {
                // Update stock
                fruits[fname].first -= qty;
                fruits[fname].second = getTimestamp();

                reply = "Purchase successful!\n";
                reply += "Remaining " + fname + " = " + to_string(fruits[fname].first) + "\n";
                reply += "Last sold = " + fruits[fname].second + "\n";
            }
        }

        reply += "Unique customers = " + to_string(unique_customers.size()) + "\n";

        // Send reply
        sendto(server_fd, reply.c_str(), reply.size(), 0,
               (struct sockaddr*)&client_addr, client_len);
    }

    close(server_fd);
    return 0;
}
