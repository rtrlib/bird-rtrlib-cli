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

/**
 * Performs cleanup on resources allocated by `init()`.
 */
void cleanup(void) {
    closelog();
}

/**
 * Initializes the application prerequisites.
 */
void init(void) {
    openlog(NULL, LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);
}

/**
 * Callback function for RTRLib that receives PFX records, translates them to
 * BIRD `add roa` commands, sends them to the BIRD server and fetches the
 * answer.
 * @param table
 * @param record
 * @param added
 */
void rtr_callback(
    struct pfx_table *table, const pfx_record record, const bool added
) {
    // IP address buffer.
    static char ip_addr_str[INET6_ADDRSTRLEN];

    // BIRD IO buffer.
    static char bird_buffer[80];

    // Fetch IP address as string.
    ip_addr_to_str(&(record.prefix), ip_addr_str, sizeof(ip_addr_str));

    // Write BIRD command to buffer.
    if (
        snprintf(
            bird_buffer,
            sizeof(bird_buffer),
            "%s roa %s/%d max %d as %d\n",
            added ? "add" : "delete",
            ip_addr_str,
            record.min_len,
            record.max_len,
            record.asn
        )
        >= sizeof(bird_buffer)
    ) {
        syslog(LOG_ERR, "BIRD command too long.");
        return;
    }

    // Log the BIRD command and send it to the BIRD server.
    syslog(LOG_INFO, "To BIRD: %s", bird_buffer);
    write(bird_socket, bird_buffer, strlen(bird_buffer));

    // Fetch the answer and log.
    bird_buffer[read(bird_socket, bird_buffer, sizeof(bird_buffer))] = 0;
    syslog(LOG_INFO, "From BIRD: %s", bird_buffer);
}

/**
 * Entry point to the BIRD RTRLib integration application.
 * @param argc
 * @param argv
 * @return
 */
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

    // Try to connect to the RTR server depending on the requested connection
    // type.
    switch (config.rtr_connection_type) {
        case tcp:
            rtr_config = rtr_tcp_connect(
                config.rtr_host, config.rtr_port, &rtr_callback
            );
            break;
        case ssh:
            rtr_config = rtr_ssh_connect(
                config.rtr_host, config.rtr_port, config.rtr_ssh_hostkey_file,
                config.rtr_ssh_username, config.rtr_ssh_privkey_file,
                config.rtr_ssh_pubkey_file, &rtr_callback
            );
            break;
    }

    // Bail out if connection cannot be established.
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
