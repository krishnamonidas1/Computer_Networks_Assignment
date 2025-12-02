#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main() {
    int sockfd;
    char buffer[1024];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    double x, y, result;
    char op[10];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    printf("UDP Scientific Calculator Server Running...\n");

    while(1) {
        addr_size = sizeof(clientAddr);
        recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&clientAddr, &addr_size);

        printf("Received: %s\n", buffer);

        result = 0;
        memset(op, 0, sizeof(op));

        // Parse expressions
        if(sscanf(buffer, "%s %lf %lf", op, &x, &y) == 3) {
            // Binary operations
            if(strcmp(op, "+") == 0) result = x + y;
            else if(strcmp(op, "-") == 0) result = x - y;
            else if(strcmp(op, "*") == 0) result = x * y;
            else if(strcmp(op, "/") == 0) result = x / y;
        }
        else if(sscanf(buffer, "%s %lf", op, &x) == 2) {
            // Unary operations
            if(strcmp(op, "sin") == 0) result = sin(x);
            else if(strcmp(op, "cos") == 0) result = cos(x);
            else if(strcmp(op, "tan") == 0) result = tan(x);
            else if(strcmp(op, "sqrt") == 0) result = sqrt(x);
            else if(strcmp(op, "inv") == 0) result = 1.0 / x;
        }

        sprintf(buffer, "Result = %lf", result);

        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr*)&clientAddr, addr_size);
    }

    close(sockfd);
    return 0;
}
