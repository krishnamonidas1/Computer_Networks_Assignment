#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main() {
    int sockfd;
    char buffer[1024];
    char expression[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("10.0.0.2");  // server IP (h2)

    while(1) {
        printf("Enter expression (e.g., sin 30, 10 + 2): ");
        fgets(expression, sizeof(expression), stdin);

        // remove newline
        expression[strcspn(expression, "\n")] = 0;

        sendto(sockfd, expression, strlen(expression), 0,
               (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        addr_size = sizeof(serverAddr);
        recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&serverAddr, &addr_size);

        printf("Server Response: %s\n\n", buffer);
    }

    close(sockfd);
    return 0;
}
