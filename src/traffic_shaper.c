#include "traffic_shaper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int apply_shaping_rules(const char *interface, int bandwidth_limit) {
    if (interface == NULL || bandwidth_limit <= 0) {
        fprintf(stderr, "Invalid parameters for applying shaping rules\n");
        return -1;
    } 

    printf("Applying shaping rule: Interface %s, Bandwidth Limit %d kbps\n", interface, bandwidth_limit);
    return 0;
}


int enable_traffic_shaping(const char *interface) {
    if (interface == NULL) {
        fprintf(stderr, "Interface cannot be NULL\n");
        return -1;
    }

    printf("Enabling traffic shaping on interface %s\n", interface);
    return 0;
}


int disable_traffic_shaping(const char *interface) {
    if (interface == NULL) {
        fprintf(stderr, "Interface cannot be NULL\n");
        return -1;
    }

    printf("Disabling traffic shaping on interface %s\n", interface);
    return 0;
}

void init_traffic_shaper() {
    printf("Traffic Shaper Initialized.\n");
}


