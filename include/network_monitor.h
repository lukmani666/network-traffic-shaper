#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <pcap.h>

typedef struct {
    unsigned long total_packets;
    unsigned long total_bytes;
} network_stats_t;

void start_network_monitor(const char *interface);
void stop_network_monitor();
void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
void display_statistics(network_stats_t *stats);
void logistics_statistics(network_stats_t *stats);

#endif