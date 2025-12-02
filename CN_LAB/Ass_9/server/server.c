// server.c
// TCP file server for upload/download with transfer time measurement.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>        // for assignment requirement (time functions)
#include <sys/time.h>    // for gettimeofday (high-resolution timing)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 4096

// Read a line ending with '\n' from socket
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

// Send all data in buffer
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

// Receive exactly len bytes
int recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    char *p = (char *)buf;

    while (total < len) {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0) {
            if (n < 0) perror("recv");
            return -1;
        }
        total += n;
    }
    return 0;
}

// Get time difference in milliseconds
double time_diff_ms(struct timeval start, struct timeval end) {
    double s = (double)start.tv_sec * 1000.0 + (double)start.tv_usec / 1000.0;
    double e = (double)end.tv_sec   * 1000.0 + (double)end.tv_usec   / 1000.0;
    return e - s;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int listen_fd, client_fd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    // Create socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;   // listen on all interfaces
    serv_addr.sin_port        = htons(port);

    // Bind
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    // Accept one client (for assignment, one transfer per run is enough)
    client_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
    if (client_fd < 0) {
        perror("accept");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected: %s\n", inet_ntoa(cli_addr.sin_addr));

    char line[512];
    ssize_t len;

    // Read command line: "GET filename\n" or "PUT filename\n"
    len = read_line(client_fd, line, sizeof(line));
    if (len <= 0) {
        fprintf(stderr, "Failed to read command from client.\n");
        close(client_fd);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    char cmd[8], filename[256];
    if (sscanf(line, "%7s %255s", cmd, filename) != 2) {
        fprintf(stderr, "Invalid command from client: %s\n", line);
        close(client_fd);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Command: %s, Filename: %s\n", cmd, filename);

    // Read size line (for both GET reply or PUT request we will use size)
    // For GET: server will send size, so we don't read here.
    // For PUT: client will send size next, so we do read it here.

    if (strcasecmp(cmd, "GET") == 0) {
        // Client wants to download file from server

        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            perror("fopen (GET)");
            // Send size -1 to indicate error
            char err_line[64];
            snprintf(err_line, sizeof(err_line), "-1\n");
            send_all(client_fd, err_line, strlen(err_line));
        } else {
            // Determine file size
            fseek(fp, 0, SEEK_END);
            long long filesize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // Send size as text
            char size_line[64];
            snprintf(size_line, sizeof(size_line), "%lld\n", filesize);
            if (send_all(client_fd, size_line, strlen(size_line)) < 0) {
                fprintf(stderr, "Failed to send size to client.\n");
                fclose(fp);
                close(client_fd);
                close(listen_fd);
                exit(EXIT_FAILURE);
            }

            // Send file content and measure time
            char buffer[BUF_SIZE];
            size_t nread;
            struct timeval start, end;
            gettimeofday(&start, NULL);

            long long total_sent = 0;
            while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                if (send_all(client_fd, buffer, nread) < 0) {
                    fprintf(stderr, "Error sending file data.\n");
                    fclose(fp);
                    close(client_fd);
                    close(listen_fd);
                    exit(EXIT_FAILURE);
                }
                total_sent += nread;
            }

            gettimeofday(&end, NULL);
            double elapsed_ms = time_diff_ms(start, end);

            printf("GET: sent %lld bytes to client.\n", total_sent);
            printf("Server side transfer time: %.3f ms\n", elapsed_ms);

            fclose(fp);
        }
    } else if (strcasecmp(cmd, "PUT") == 0) {
        // Client wants to upload file to server

        // Next line from client: file size as text
        len = read_line(client_fd, line, sizeof(line));
        if (len <= 0) {
            fprintf(stderr, "Failed to read size from client.\n");
            close(client_fd);
            close(listen_fd);
            exit(EXIT_FAILURE);
        }

        long long filesize = atoll(line);
        if (filesize < 0) {
            fprintf(stderr, "Client reported invalid file size.\n");
            close(client_fd);
            close(listen_fd);
            exit(EXIT_FAILURE);
        }

        FILE *fp = fopen(filename, "wb");
        if (!fp) {
            perror("fopen (PUT)");
            close(client_fd);
            close(listen_fd);
            exit(EXIT_FAILURE);
        }

        char buffer[BUF_SIZE];
        long long remaining = filesize;
        struct timeval start, end;
        gettimeofday(&start, NULL);

        while (remaining > 0) {
            ssize_t to_read = (remaining > BUF_SIZE) ? BUF_SIZE : remaining;
            ssize_t n = recv(client_fd, buffer, to_read, 0);
            if (n <= 0) {
                if (n < 0) perror("recv");
                fprintf(stderr, "Connection closed or error during PUT.\n");
                fclose(fp);
                close(client_fd);
                close(listen_fd);
                exit(EXIT_FAILURE);
            }
            fwrite(buffer, 1, n, fp);
            remaining -= n;
        }

        gettimeofday(&end, NULL);
        double elapsed_ms = time_diff_ms(start, end);

        printf("PUT: received %lld bytes from client.\n", filesize);
        printf("Server side transfer time: %.3f ms\n", elapsed_ms);

        fclose(fp);
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
    }

    close(client_fd);
    close(listen_fd);
    return 0;
}
