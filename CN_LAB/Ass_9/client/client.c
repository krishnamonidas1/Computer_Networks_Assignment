// client.c
// TCP file client for uploading/downloading files with transfer time measurement.
//
// Usage:
//   Download (client receives file from server):
//     ./client <server_ip> <port> get <remote_filename> <local_save_as>
//
//   Upload (client sends file to server):
//     ./client <server_ip> <port> put <local_file> <remote_filename_on_server>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>        // for assignment requirement
#include <sys/time.h>    // for gettimeofday
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 4096

ssize_t read_line(int sock, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;
    ssize_t n;

    while (i < maxlen - 1) {
        n = recv(sock, &c, 1, 0);
        if (n <= 0) {
            if (n < 0) perror("recv");
            break;
        }
        buf[i++] = c;
        if (c == '\n') break;
    }

    buf[i] = '\0';
    return (ssize_t)i;
}

int send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = (const char *)buf;

    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) {
            if (n < 0) perror("send");
            return -1;
        }
        total += n;
    }
    return 0;
}

double time_diff_ms(struct timeval start, struct timeval end) {
    double s = (double)start.tv_sec * 1000.0 + (double)start.tv_usec / 1000.0;
    double e = (double)end.tv_sec * 1000.0 + (double)end.tv_usec / 1000.0;
    return e - s;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr,
            "Usage:\n"
            "  Download: %s <server_ip> <port> get <remote_file> <local_file>\n"
            "  Upload  : %s <server_ip> <port> put <local_file> <remote_file>\n",
            argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int port        = atoi(argv[2]);
    char *mode      = argv[3];

    int sockfd;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", server_ip, port);

    if (strcasecmp(mode, "get") == 0) {
        // Download from server to client
        // remote_file: argv[4], local_file: argv[5]
        char *remote_file = argv[4];
        char *local_file  = argv[5];

        // Send command: "GET remote_file\n"
        char cmd_line[512];
        snprintf(cmd_line, sizeof(cmd_line), "GET %s\n", remote_file);
        if (send_all(sockfd, cmd_line, strlen(cmd_line)) < 0) {
            fprintf(stderr, "Failed to send GET command.\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Read file size from server
        char line[128];
        ssize_t len = read_line(sockfd, line, sizeof(line));
        if (len <= 0) {
            fprintf(stderr, "Failed to read file size from server.\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        long long filesize = atoll(line);
        if (filesize < 0) {
            fprintf(stderr, "Server reported error opening file.\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("GET: server file size = %lld bytes\n", filesize);

        FILE *fp = fopen(local_file, "wb");
        if (!fp) {
            perror("fopen (local_file)");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        char buffer[BUF_SIZE];
        long long remaining = filesize;
        struct timeval start, end;
        gettimeofday(&start, NULL);

        while (remaining > 0) {
            ssize_t to_read = (remaining > BUF_SIZE) ? BUF_SIZE : remaining;
            ssize_t n = recv(sockfd, buffer, to_read, 0);
            if (n <= 0) {
                if (n < 0) perror("recv");
                fprintf(stderr, "Connection closed or error while downloading.\n");
                fclose(fp);
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            fwrite(buffer, 1, n, fp);
            remaining -= n;
        }

        gettimeofday(&end, NULL);
        double elapsed_ms = time_diff_ms(start, end);

        printf("Downloaded %lld bytes from server.\n", filesize);
        printf("Client side transfer time (download): %.3f ms\n", elapsed_ms);

        fclose(fp);

    } else if (strcasecmp(mode, "put") == 0) {
        // Upload from client to server
        // local_file: argv[4], remote_file: argv[5]
        char *local_file  = argv[4];
        char *remote_file = argv[5];

        FILE *fp = fopen(local_file, "rb");
        if (!fp) {
            perror("fopen (local_file)");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Determine file size
        fseek(fp, 0, SEEK_END);
        long long filesize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        printf("PUT: local file size = %lld bytes\n", filesize);

        // Send command: "PUT remote_file\n"
        char cmd_line[512];
        snprintf(cmd_line, sizeof(cmd_line), "PUT %s\n", remote_file);
        if (send_all(sockfd, cmd_line, strlen(cmd_line)) < 0) {
            fprintf(stderr, "Failed to send PUT command.\n");
            fclose(fp);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Send file size as text
        char size_line[64];
        snprintf(size_line, sizeof(size_line), "%lld\n", filesize);
        if (send_all(sockfd, size_line, strlen(size_line)) < 0) {
            fprintf(stderr, "Failed to send file size.\n");
            fclose(fp);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Send file content and measure time
        char buffer[BUF_SIZE];
        size_t nread;
        struct timeval start, end;
        gettimeofday(&start, NULL);

        long long total_sent = 0;
        while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            if (send_all(sockfd, buffer, nread) < 0) {
                fprintf(stderr, "Error sending file data.\n");
                fclose(fp);
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            total_sent += nread;
        }

        gettimeofday(&end, NULL);
        double elapsed_ms = time_diff_ms(start, end);

        printf("Uploaded %lld bytes to server.\n", total_sent);
        printf("Client side transfer time (upload): %.3f ms\n", elapsed_ms);

        fclose(fp);

    } else {
        fprintf(stderr, "Unknown mode: %s (use get or put)\n", mode);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return 0;
}

