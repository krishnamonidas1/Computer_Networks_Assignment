#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>        // struct ip
#include <netinet/tcp.h>       // struct tcphdr
#include <netinet/udp.h>       // struct udphdr
#include <netinet/ip_icmp.h>   // struct icmphdr
#include <netinet/if_ether.h>  // struct ether_header

// Flags to remember which protocols we saw
int seen_eth = 0;
int seen_arp = 0;
int seen_ipv4 = 0;
int seen_ipv6 = 0;
int seen_icmp = 0;
int seen_tcp = 0;
int seen_udp = 0;

// Helper to print IP address nicely
void print_ip(const char *label, struct in_addr addr) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, buf, sizeof(buf));
    printf("%s%s", label, buf);
}

int main(int argc, char *argv[])
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    const u_char *packet;
    struct pcap_pkthdr header;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pcap_file>\n", argv[0]);
        return 1;
    }

    // Open the pcap file offline
    handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", argv[1], errbuf);
        return 2;
    }

    printf("Reading packets from %s\n\n", argv[1]);

    // To compute relative time
    int first = 1;
    struct timeval t0 = {0, 0};

    while ((packet = pcap_next(handle, &header)) != NULL) {
        // Get timestamp
        double rel_time;
        if (first) {
            t0 = header.ts;
            first = 0;
        }
        rel_time = (header.ts.tv_sec - t0.tv_sec)
                 + (header.ts.tv_usec - t0.tv_usec) / 1000000.0;

        // Parse Ethernet header
        const struct ether_header *eth;
        eth = (struct ether_header *)packet;
        seen_eth = 1;

        u_short ether_type = ntohs(eth->ether_type);

        // Time diagram line starts here
        printf("[%.6f] ", rel_time);

        if (ether_type == ETHERTYPE_IP) {
            // IPv4
            seen_ipv4 = 1;
            printf("Ethernet -> IPv4 ");

            const struct ip *ip_hdr;
            ip_hdr = (struct ip *)(packet + sizeof(struct ether_header));

            print_ip("Src=", ip_hdr->ip_src);
            printf("  ");
            print_ip("Dst=", ip_hdr->ip_dst);
            printf("  ");

            // Check L4 protocol
            switch (ip_hdr->ip_p) {
                case IPPROTO_ICMP: {
                    seen_icmp = 1;
                    printf("Proto=ICMP ");
                    const struct icmphdr *icmp_hdr =
                        (struct icmphdr *)((u_char *)ip_hdr + ip_hdr->ip_hl * 4);
                    printf("Type=%d Code=%d", icmp_hdr->type, icmp_hdr->code);
                    break;
                }
                case IPPROTO_TCP: {
                    seen_tcp = 1;
                    printf("Proto=TCP ");
                    const struct tcphdr *tcp_hdr =
                        (struct tcphdr *)((u_char *)ip_hdr + ip_hdr->ip_hl * 4);
                    printf("SrcPort=%u DstPort=%u",
                           ntohs(tcp_hdr->source),
                           ntohs(tcp_hdr->dest));
                    break;
                }
                case IPPROTO_UDP: {
                    seen_udp = 1;
                    printf("Proto=UDP ");
                    const struct udphdr *udp_hdr =
                        (struct udphdr *)((u_char *)ip_hdr + ip_hdr->ip_hl * 4);
                    printf("SrcPort=%u DstPort=%u",
                           ntohs(udp_hdr->source),
                           ntohs(udp_hdr->dest));
                    break;
                }
                default:
                    printf("Proto=OTHER(%d)", ip_hdr->ip_p);
                    break;
            }
        } else if (ether_type == ETHERTYPE_ARP) {
            seen_arp = 1;
            printf("Ethernet -> ARP");
        } else if (ether_type == ETHERTYPE_IPV6) {
            seen_ipv6 = 1;
            printf("Ethernet -> IPv6 (not decoded in detail here)");
        } else {
            printf("Ethernet -> Other(0x%04x)", ether_type);
        }

        printf("\n");
    }

    pcap_close(handle);

    printf("\n=============================\n");
    printf("Unique protocols observed:\n");
    printf("=============================\n");

    // L2
    printf("Layer 2 (Data Link):\n");
    if (seen_eth) printf("  - Ethernet\n");
    if (seen_arp) printf("  - ARP\n");

    // L3
    printf("Layer 3 (Network):\n");
    if (seen_ipv4) printf("  - IPv4\n");
    if (seen_ipv6) printf("  - IPv6\n");
    if (seen_icmp) printf("  - ICMP (control, often considered part of L3)\n");

    // L4
    printf("Layer 4 (Transport):\n");
    if (seen_tcp) printf("  - TCP\n");
    if (seen_udp) printf("  - UDP\n");

    printf("\nNote: For a pure PING capture you will at least see Ethernet, ARP, IPv4, and ICMP.\n");

    return 0;
}

