#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "cli.h"
#include "config.h"
#include "rtr.h"

#define CMD_EXIT "exit"

// Socket to BIRD.
static int bird_socket = -1;

// RTR manager config.
static struct rtr_mgr_config *rtr_config = 0;

void cleanup(void) {
    closelog();
}

void init(void) {
    openlog(NULL, LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);
}

void rtr_callback(
    struct pfx_table *table, const pfx_record record, const bool added
) {
    // TODO: dummy.
    char ip[INET6_ADDRSTRLEN];
    ip_addr_to_str(&(record.prefix), ip, sizeof(ip));
    fprintf(stderr, "%s %-18s %3u-%-3u %10u\n", added ? "+" : "-", ip, record.min_len, record.max_len, record.asn);
}

int main(int argc, char *argv[]) {
    // Main configuration.
    struct config config;
    
    // Buffer for commands and its length.
    char *command = 0;
    size_t command_len = 0;
    
    // Initialize variables.
    config_init(&config);
    
    // Initialize framework.
    init();
    
    // Parse CLI arguments into config and bail out on error.
    if (!parse_cli(argc, argv, &config)) {
        cleanup();
        return EXIT_FAILURE;
    }
    
    // Check config.
    if (!config_check(&config)) {
        cleanup();
        return EXIT_FAILURE;
    }
    
    // Try to connect to BIRD and bail out on failure.
    bird_socket = bird_connect(config.bird_socket_path);
    if (bird_socket == -1) {
        cleanup();
        return EXIT_FAILURE;
    }
    
    // Try to connect to the RTR server and bail out on failure.
    rtr_config = rtr_connect(config.rtr_host, config.rtr_port, &rtr_callback);
    if (!rtr_config) {
        cleanup();
        return EXIT_FAILURE;
    };
    
    // Server loop. Read commands from stdin.
    while (getline(&command, &command_len, stdin) != -1) {
        if (strncmp(command, CMD_EXIT, strlen(CMD_EXIT)) == 0)
            break;
    }
    
    // Clean up RTRLIB memory.
    rtr_close(rtr_config);
    
    // Close BIRD socket.
    close(bird_socket);
    
    // Cleanup framework.
    cleanup();
    
    // Exit with success.
    return EXIT_SUCCESS;
}
