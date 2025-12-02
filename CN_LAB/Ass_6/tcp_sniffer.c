#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

int main() {
    int sock_raw;
    ssize_t data_size;
    unsigned char buffer[65536];

    // 1. Create raw socket to capture all packets
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        perror("Socket Error");
        return 1;
    }

    printf("Listening for incoming packets...\n");

    while (1) {
        // 2. Receive a packet
        data_size = recvfrom(sock_raw, buffer, sizeof(buffer), 0, NULL, NULL);
        if (data_size < 0) {
            perror("Recvfrom error");
            return 1;
        }

        // Pointer to Ethernet header
        struct ethhdr *eth = (struct ethhdr *)(buffer);

        // Only process IPv4 packets (ETH_P_IP == 0x0800)
        if (ntohs(eth->h_proto) == ETH_P_IP) {
            struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

            // If protocol = TCP (6)
            if (ip->protocol == 6) {
                struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + ip->ihl*4);

                struct sockaddr_in src, dst;
                src.sin_addr.s_addr = ip->saddr;
                dst.sin_addr.s_addr = ip->daddr;

                printf("\n--- TCP Packet Captured ---\n");
                printf("Packet Size: %ld bytes\n", data_size);

                // IP header info
                printf("Source IP: %s\n", inet_ntoa(src.sin_addr));
                printf("Destination IP: %s\n", inet_ntoa(dst.sin_addr));

                // TCP header info
                printf("Source Port: %u\n", ntohs(tcp->source));
                printf("Destination Port: %u\n", ntohs(tcp->dest));
                printf("Sequence Number: %u\n", ntohl(tcp->seq));
                printf("Acknowledgement: %u\n", ntohl(tcp->ack_seq));
                printf("Flags: SYN=%d ACK=%d FIN=%d RST=%d\n",
                       tcp->syn, tcp->ack, tcp->fin, tcp->rst);
                printf("---------------------------\n");
            }
        }
    }

    close(sock_raw);
    return 0;
}
