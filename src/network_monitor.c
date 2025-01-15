#include "network_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <time.h>

#ifndef u_char
typedef unsigned char u_char;
#endif

static pcap_t *handle;
static network_stats_t stats;

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    struct iphdr *ip_header = (struct iphdr *)(packet + 14);

    stats.total_packets++;
    stats.total_bytes += header->len;

    printf("Packet Captured: length %d\n", header->len);
    printf("Source IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    printf("Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->daddr));

    if (ip_header->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp_header = (struct tcphdr *)(packet + 14 + (ip_header->ihl * 4));
        if (ntohs(tcp_header->dest) == 80) {
            printf("TCP packet on port 80 captured: length %d\n", header->len);
            printf("source IP: %s, Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr), inet_ntoa(*(struct in_addr *)&ip_header->daddr));
        }
        printf("TCP packet: Source port: %d, Destination port: %d\n", ntohs(tcp_header->source), ntohs(tcp_header->dest));
    } else if (ip_header->protocol == IPPROTO_UDP) {
        struct udphdr *udp_header = (struct udphdr *)(packet + 14 + (ip_header->ihl * 4));
        printf("UDP packet: Source port: %d, Destination port: %d\n", ntohs(udp_header->source), ntohs(udp_header->dest));
    }

    printf("----\n");
}

void display_statistics(network_stats_t *stats) {
    printf("Total packets captured: %lu\n", stats->total_packets);
    printf("Total bytes captured: %lu\n", stats->total_bytes);
}

void logistics_statistics(network_stats_t *stats) {
    FILE *log_file = fopen("network_log.txt", "a");

    if (log_file == NULL) {
        fprintf(stderr, "Could not open log file writing.\n");
        return;
    }

    time_t now = time(NULL);
    fprintf(log_file, "Time: %s", ctime(&now));
    fprintf(log_file, "Total packets captured: %lu\n", stats->total_packets);
    fprintf(log_file, "Total bytes captured: %lu\n", stats->total_bytes);
    fprintf(log_file, "----\n");

    fclose(log_file);
}

void start_network_monitor(const char *interface) {
    char error_buffer[PCAP_ERRBUF_SIZE];

    handle = pcap_open_live(interface, BUFSIZ, 1, 1000, error_buffer);
    if (handle == NULL) {
        fprintf(stderr, "Could not open device %s: %s\n", interface, error_buffer);
        return;
    }

    printf("Monitoring interface %s...\n", interface);

    pcap_loop(handle, 0, packet_handler, NULL);
}

void stop_network_monitor() {
    if (handle) {
        pcap_breakloop(handle);
        pcap_close(handle);
        printf("Stopped network monitoring.\n");

        display_statistics(&stats);

        logistics_statistics(&stats);
    }
}

