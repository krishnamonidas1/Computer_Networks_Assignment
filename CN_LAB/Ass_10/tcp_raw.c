// tcp_raw.c
// Send one custom TCP packet with payload containing your roll number
// Usage: sudo ./tcp_raw <src_ip> <dst_ip> <dst_port> <roll_number_string>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

// Pseudo header for TCP checksum
struct pseudo_header {
    u_int32_t src_addr;
    u_int32_t dst_addr;
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t tcp_length;
};

// Generic checksum function
unsigned short checksum(unsigned short *ptr, int nbytes) {
    long sum = 0;
    unsigned short oddbyte;
    unsigned short answer;

    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }

    // Fold 32-bit sum to 16 bits
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (unsigned short)~sum;

    return answer;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: sudo %s <src_ip> <dst_ip> <dst_port> <roll_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *src_ip_str  = argv[1];
    char *dst_ip_str  = argv[2];
    int dst_port      = atoi(argv[3]);
    char *roll_str    = argv[4];

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Tell the kernel we will provide IP header
    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt IP_HDRINCL");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Buffer for IP header + TCP header + payload
    char packet[4096];
    memset(packet, 0, sizeof(packet));

    // IP header
    struct iphdr *iph = (struct iphdr *)packet;
    // TCP header comes after IP header
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // Payload (roll number)
    char *data = packet + sizeof(struct iphdr) + sizeof(struct tcphdr);
    int data_len = strlen(roll_str);
    memcpy(data, roll_str, data_len);

    // Fill in IP header
    iph->ihl = 5;                      // Header length
    iph->version = 4;                  // IPv4
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len);
    iph->id = htons(54321);            // Any ID
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;                    // Will compute later
    iph->saddr = inet_addr(src_ip_str);
    iph->daddr = inet_addr(dst_ip_str);

    // Fill in TCP header
    tcph->source = htons(1234);        // Arbitrary source port
    tcph->dest = htons(dst_port);
    tcph->seq = htonl(1);
    tcph->ack_seq = 0;
    tcph->doff = 5;                    // TCP header size
    tcph->syn = 1;                     // Just mark SYN (or data packet, as you like)
    tcph->ack = 0;
    tcph->fin = 0;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->urg = 0;
    tcph->window = htons(5840);        // Arbitrary window size
    tcph->check = 0;                   // Will compute later
    tcph->urg_ptr = 0;

    // Compute IP checksum
    iph->check = checksum((unsigned short *)packet, sizeof(struct iphdr));

    // TCP checksum needs pseudo header
    struct pseudo_header psh;
    psh.src_addr = iph->saddr;
    psh.dst_addr = iph->daddr;
    psh.reserved = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + data_len);

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + data_len;
    char *pseudo_packet = malloc(psize);
    if (!pseudo_packet) {
        perror("malloc");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memcpy(pseudo_packet, &psh, sizeof(struct pseudo_header));
    memcpy(pseudo_packet + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + data_len);

    tcph->check = checksum((unsigned short *)pseudo_packet, psize);
    free(pseudo_packet);

    // Destination address
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(dst_port);
    dest.sin_addr.s_addr = iph->daddr;

    if (sendto(sockfd, packet, sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len,
               0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Custom TCP packet sent to %s:%d with payload: \"%s\"\n",
           dst_ip_str, dst_port, roll_str);

    close(sockfd);
    return 0;
}
