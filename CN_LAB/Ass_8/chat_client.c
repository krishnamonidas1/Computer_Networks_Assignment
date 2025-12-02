#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9000

int sock;

// Thread to receive messages
void *receive_handler(void *arg) {
    char buffer[1024];
    int len;

    while((len = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[len] = '\0';
        printf("%s", buffer);
    }

    return NULL;
}

int main() {
    struct sockaddr_in server;
    char message[1024];
    pthread_t recv_thread;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("Socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("10.0.0.2"); // server IP

    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        return 1;
    }

    printf("Connected to chat server...\n");

    pthread_create(&recv_thread, NULL, receive_handler, NULL);

    while(1) {
        fgets(message, sizeof(message), stdin);
        send(sock, message, strlen(message), 0);
    }

    close(sock);
    return 0;
}
