// icmp_timestamp.c
// Send one ICMP Timestamp request (Type 13) using raw socket
// Usage: sudo ./icmp_timestamp <src_ip> <dst_ip>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>

// ICMP Timestamp has: type, code, checksum, id, seq, originate, receive, transmit
struct icmp_timestamp {
    u_int8_t  type;
    u_int8_t  code;
    u_int16_t checksum;
    u_int16_t id;
    u_int16_t sequence;
    u_int32_t originate;
    u_int32_t receive;
    u_int32_t transmit;
};

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

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (unsigned short)~sum;
    return answer;
}

// Simple function to get milliseconds since epoch (not exactly per RFC, but fine for demo)
u_int32_t get_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    u_int32_t ms = (u_int32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
    return ms;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: sudo %s <src_ip> <dst_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *src_ip_str = argv[1];
    char *dst_ip_str = argv[2];

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt IP_HDRINCL");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char packet[4096];
    memset(packet, 0, sizeof(packet));

    struct iphdr *iph = (struct iphdr *)packet;
    struct icmp_timestamp *icmph = (struct icmp_timestamp *)(packet + sizeof(struct iphdr));

    // Fill IP header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmp_timestamp));
    iph->id = htons(12345);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_ICMP;
    iph->check = 0;
    iph->saddr = inet_addr(src_ip_str);
    iph->daddr = inet_addr(dst_ip_str);
    iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr));

    // Fill ICMP Timestamp header
    icmph->type = 13;      // Timestamp request
    icmph->code = 0;
    icmph->checksum = 0;
    icmph->id = htons(1);
    icmph->sequence = htons(1);

    // For assignment demo, we can set only originate; others zero
    u_int32_t now_ms = get_ms();
    icmph->originate = htonl(now_ms);
    icmph->receive = 0;
    icmph->transmit = 0;

    // Compute ICMP checksum
    icmph->checksum = checksum((unsigned short *)icmph, sizeof(struct icmp_timestamp));

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = iph->daddr;

    if (sendto(sockfd, packet, sizeof(struct iphdr) + sizeof(struct icmp_timestamp),
               0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("ICMP Timestamp request sent to %s\n", dst_ip_str);

    close(sockfd);
    return 0;
}

