#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 50

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock;

// Broadcast message to all clients
void broadcast(char *msg, int sender_fd) {
    pthread_mutex_lock(&lock);

    for(int i = 0; i < client_count; i++) {
        if(clients[i] != sender_fd) {
            send(clients[i], msg, strlen(msg), 0);
        }
    }

    pthread_mutex_unlock(&lock);
}

// Save message to log.txt
void write_log(char *msg) {
    FILE *fp = fopen("log.txt", "a");
    if(fp == NULL) return;

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = 0; // remove newline

    fprintf(fp, "[%s] %s", timestamp, msg);
    fclose(fp);
}

// Thread function to handle individual client
void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char msg[1024];
    int read_size;

    while((read_size = recv(sock, msg, sizeof(msg), 0)) > 0) {
        msg[read_size] = '\0';  // null terminate

        write_log(msg);
        broadcast(msg, sock);
    }

    // Remove client when disconnected
    pthread_mutex_lock(&lock);
    for(int i = 0; i < client_count; i++) {
        if(clients[i] == sock) {
            clients[i] = clients[client_count-1];
            break;
        }
    }
    client_count--;
    pthread_mutex_unlock(&lock);

    close(sock);
    pthread_exit(NULL);
}

int main() {
    int sockfd, newfd;
    struct sockaddr_in server, client;
    socklen_t c = sizeof(client);
    pthread_t thread_id;

    pthread_mutex_init(&lock, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("Socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(sockfd, 10);
    printf("Chat server started on port %d...\n", PORT);

    while((newfd = accept(sockfd, (struct sockaddr*)&client, &c))) {
        printf("Client connected: %d\n", newfd);

        pthread_mutex_lock(&lock);
        clients[client_count++] = newfd;
        pthread_mutex_unlock(&lock);

        if(pthread_create(&thread_id, NULL, client_handler, (void*)&newfd) < 0) {
            perror("Thread error");
            return 1;
        }
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
