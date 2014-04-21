#include <stdio.h>
#include <string.h>

#include "config.h"

int config_check(const struct config *config) {
    // Check BIRD control socket path availability.
    if (!config->bird_socket_path) {
        fprintf(stderr, "Missing path to BIRD control socket.");
        return 0;
    }
    
    // Check RTR host availability.
    if (!config->rtr_host) {
        fprintf(stderr, "Missing RTR server host.");
        return 0;
    }
    
    // Check RTR port availability.
    if (!config->rtr_port) {
        fprintf(stderr, "Missing RTR server port.");
        return 0;
    }
    
    // Return success.
    return 1;
}

void config_init(struct config *config) {
    memset(config, 0, sizeof (struct config));
}
