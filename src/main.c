#include "traffic_shaper.h"
#include "network_monitor.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <microhttpd.h>
#include <stdlib.h>

#define STATIC_DIR "www/"
// #define PORT 8888

static volatile int keep_running = 1;


void handle_signal(int sig) {
    keep_running = 0;
    stop_network_monitor();
}

int monitoring_active = 0;

struct RequestData {
    struct MHD_PostProcessor *post_processor;
    char *limit;
};

static enum MHD_Result post_iterator(void *coninfo_cls, enum MHD_ValueKind Kind, const char *key, const char *filename,
                        const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
    struct RequestData *request_data = coninfo_cls;
    if (strcmp(key, "limit") == 0) {
        request_data->limit = strndup(data, size);
    }

    return MHD_YES;

}


static enum MHD_Result serve_html(const char *url, struct MHD_Connection *connection) {
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", STATIC_DIR, url);

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        const char *not_found = "<html><body><h1>404 Not Found</h1></body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(not_found), (void *)not_found, MHD_RESPMEM_PERSISTENT);
        // int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        // MHD_destroy_response(response);
        // return ret;

        if (response == NULL) {
            return MHD_NO; // Return MHD_NO if creating the response failed
        }

        MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return MHD_YES;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *content = malloc(length);
    if (content) {
        fread(content, 1, length, file);
        fclose(file);

        struct MHD_Response *response = MHD_create_response_from_buffer(length, content, MHD_RESPMEM_MUST_FREE);
        if (response == NULL) {
            free(content);
            return MHD_NO;
        }
        MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return MHD_YES;
    } else {
        fclose(file);
        const char *error_msg = "<html><body><h1>500 Internal Server Error</h1></body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_msg), (void *)error_msg, MHD_RESPMEM_PERSISTENT);
        if (response == NULL) {
            return MHD_NO;
        }
        MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return MHD_YES;
    }
}



static enum MHD_Result answer_to_request(void *cls, struct MHD_Connection *connection, const char *url,
                            const char *method, const char *version, const char *upload_data,
                            size_t *upload_data_size, void **con_cls) {
                            const char *response = "<html><body><h1>Traffic Shaper & Network Monitor</h1>";

                            // const char *html = STATIC_DIR;
                            static int aptr;
                            if (&aptr != *con_cls) {
                                *con_cls = &aptr;
                                return MHD_YES;
                            }

                            if (*con_cls == NULL) {
                                struct RequestData *request_data = malloc(sizeof(struct RequestData));
                                request_data->limit = NULL;
                                *con_cls = (void *)request_data;
                                return MHD_YES;
                            }

                            struct RequestData *request_data = *con_cls;

                            if (strcmp(method, "POST") == 0) {
                                struct MHD_PostProcessor *pp = MHD_create_post_processor(connection, 1024, post_iterator, request_data);

                                if (pp == NULL) {
                                    return MHD_NO;
                                }

                                if (*upload_data_size != 0) {
                                    MHD_post_process(pp, upload_data, *upload_data_size);
                                    *upload_data_size = 0;
                                    return MHD_YES;
                                }

                                MHD_destroy_post_processor(pp);
                            }

                            if (strcmp(url, "/start-network-monitoring") == 0) {
                                monitoring_active = 1;
                                start_network_monitor("wlo1");

                                enum MHD_Result ret = serve_html("index.html", connection);

                                if (ret == MHD_NO) {
                                    return MHD_NO;
                                }

                                return MHD_YES;

                                // if (html_response == NULL) {
                                //     return MHD_NO;
                                // }

                                // enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, html_response);
                                // MHD_destroy_response(html_response);
                                // return ret;
                                // return MHD_queue_response(
                                //     connection, MHD_HTTP_OK, 
                                //     MHD_create_response_from_buffer(strlen("Monitoring started."),
                                //     (void*)"Monitoring started.", MHD_RESPMEM_PERSISTENT)
                                // );
                            }

                            if (strcmp(url, "/stop-network-monitoring") == 0) {
                                monitoring_active = 0;
                                stop_network_monitor();
                                return MHD_queue_response(
                                    connection, MHD_HTTP_OK, 
                                    MHD_create_response_from_buffer(strlen("Monitoring stopped."),
                                    (void*)"Monitoring stopped.", MHD_RESPMEM_PERSISTENT)
                                );
                            }

                            if (strcmp(url, "/get-stats") == 0 && strcmp(method, "GET") == 0) {
                                const char *stats = monitoring_active ? "Monitoring is active" : "Monitoring is not active";
                                char response[256];
                                snprintf(response, sizeof(response), "{\"stats\":\"%s\"}", stats);

                                struct MHD_Response *mhd_response = MHD_create_response_from_buffer(
                                    strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT
                                );
                                
                                MHD_add_response_header(mhd_response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");

                                int ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);

                                MHD_destroy_response(mhd_response);

                                return ret;

                            }

                            if (strcmp(url, "/apply-limit") == 0) {

                                if (request_data->limit) {
                                    int limit = atoi(request_data->limit);
                                    char command[256];
                                    snprintf(command, sizeof(command),
                                    "tc qdisc add dev wlo1 root tbf rate %dkbps burst 32kbit latency 400ms", limit
                                    );
                                    system(command);
                                    free(request_data->limit);
                                }

                                return MHD_queue_response(connection, MHD_HTTP_OK, MHD_create_response_from_buffer(strlen("Limit applied."), (void*)"Limit applied.", MHD_RESPMEM_PERSISTENT));
    
                            }

                            if (strncmp(url, "/static/", 8) == 0 || strstr(url, ".css") || strstr(url, ".js")) {
                                // return serve_html(url, connection);
                                // struct MHD_Response *html_response = serve_html(url, connection);
                                // if (html_response == NULL) {
                                //     return MHD_NO;
                                // }
                                enum MHD_Result ret = serve_html("index.html", connection);

                                if (ret == MHD_NO) {
                                    return MHD_NO;
                                }

                                return MHD_YES;

                                // enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, html_response);
                                // MHD_destroy_response(html_response);
                                // return MHD_YES;
                            
                            }

                            if (strcmp(url, "/") == 0) {
                                // struct MHD_Response *html_response = serve_html("index.html", connection);
                                // if (html_response == NULL) {
                                //     return MHD_NO;
                                // }

                                enum MHD_Result ret = serve_html("index.html", connection);

                                if (ret == MHD_NO) {
                                    return MHD_NO;
                                }

                                return MHD_YES;

                                // enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, html_response);
                                // MHD_destroy_response(html_response);
                                // return ret;
                            } 
                            // else if (strcmp(url, "/start") == 0) {
                            //     start_network_monitor("wlo1");
                            //     return serve_html("index.html", connection);
                            // } else if (strcmp(url, "/stop") == 0) {
                            //     stop_network_monitor();
                            //     return serve_html("index.html", connection);
                            else {
                                return MHD_NO;
                            }
                            struct MHD_Response *mhd_response = MHD_create_response_from_buffer(strlen(response), (void *)response, MHD_RESPMEM_PERSISTENT);
                            int ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
                            MHD_destroy_response(mhd_response);
                            return ret;
}




int main() {
    load_config("config.json");

    init_traffic_shaper();

    const char *interface = "wlo1";
    int bandwidth_limit = get_bandwidth_limit(interface);
    printf("Bandwidth Limit for %s: %d\n", interface, bandwidth_limit);

    if (bandwidth_limit > 0) {
        apply_shaping_rules(interface, bandwidth_limit);
    } else {
        fprintf(stderr, "Invalid bandwidth limit for interface %s\n", interface);
    }

    enable_traffic_shaping(interface);

    signal(SIGINT, handle_signal);

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, 9090, NULL, NULL, 
                                                  &answer_to_request, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start web server\n");
        return 1;
    }

    printf("Web server started at http://127.0.0.1:9090\n");

    static network_stats_t stats = {0};

    printf("Starting network monitoring on %s. Press Ctrl+C to stop.\n", interface);
    start_network_monitor(interface);

    while (keep_running) {
        sleep(5);

        display_statistics(&stats);
        logistics_statistics(&stats);
    }

    stop_network_monitor();
    MHD_stop_daemon(daemon);

    return 0;
}