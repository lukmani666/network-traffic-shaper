#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>


typedef struct {
    char interface[256];
    int bandwidth_limit;
} InterfaceConfig;

static InterfaceConfig interface_config[10];
static int num_interfaces = 0;

void load_config(const char *config_file) {
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        perror("Error opening config file");
        return;
    }

    struct json_object *parsed_json;
    struct json_object *interfaces;
    parsed_json = json_object_from_file(config_file);
    
    if (!parsed_json) {
        fprintf(stderr, "Error parsing config file\n");
        fclose(fp);
        return;
    }

    interfaces = json_object_object_get(parsed_json, "interfaces");
    if (interfaces) {
        json_object_object_foreach(interfaces, key, value) {
            strncpy(interface_config[num_interfaces].interface, key, sizeof(interface_config[num_interfaces].interface));
            interface_config[num_interfaces].bandwidth_limit = json_object_get_int(json_object_object_get(value, "bandwidth_limit"));
            num_interfaces++;
        }
    }

    fclose(fp);
}


int get_bandwidth_limit(const char *interface) {
    for (int i = 0; i < num_interfaces; i++) {
        if (strcmp(interface_config[i].interface, interface) == 0) {
            return interface_config[i].bandwidth_limit;
        }
    }

    return -1;
}