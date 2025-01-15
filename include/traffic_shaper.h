#ifndef TRAFFIC_SHAPER_H
#define TRAFFIC_SHAPER_H

void init_traffic_shaper();
int apply_shaping_rules(const char *interface, int bandwidth_limit);
int enable_traffic_shaping(const char *interface);
int disable_traffic_shaping(const char *interface);

#endif