/*
 * This file is part of BIRD-RTRlib-CLI.
 *
 * BIRD-RTRlib-CLI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * BIRD-RTRlib-CLI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with BIRD-RTRlib-CLI; see the file COPYING.
 *
 * written by Mehmet Ceyran, in cooperation with:
 * CST group, Freie Universitaet Berlin
 * Website: https://github.com/rtrlib/bird-rtrlib-cli
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <netinet/in.h>

#include "cli.h"
#include "config.h"
#include "rtr.h"

#define CMD_EXIT "exit"

// Socket to BIRD.
static int bird_socket = -1;

// Buffer for BIRD commands.
static char *bird_command = 0;

// Length of buffer for BIRD commands.
static size_t bird_command_length = -1;

// "add roa" BIRD command "table" part. Defaults to an empty string and becomes
// "table " + config->bird_roa_table if provided.
static char *bird_add_roa_table_arg = "";

// RTR manager config.
static struct rtr_mgr_config *rtr_config = 0;

/**
 * Performs cleanup on resources allocated by `init()`.
 */
void cleanup(void) {
    closelog();
}

/**
 * Frees memory allocated with the " table <bird_roa_table>" clause for the
 * "add roa" BIRD command.
 */
void cleanup_bird_add_roa_table_arg(void) {
    // If the buffer is "", it has never been changed, thus there is no malloc'd
    // buffer.
    if (strcmp(bird_add_roa_table_arg, "") != 0)
        free(bird_add_roa_table_arg);
}

/**
 * Frees memory allocated with the BIRD command buffer.
 */
void cleanup_bird_command(void) {
    if (bird_command) {
        free(bird_command);
        bird_command = 0;
        bird_command_length = -1;
    }
}

/**
 * Initializes the application prerequisites.
 */
void init(void) {
    openlog(NULL, LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);
}

/**
 * Creates and populates the "add roa" command's "table" argument buffer.
 * @param bird_roa_table
 */
void init_bird_add_roa_table_arg(char *bird_roa_table) {
    // Size of the buffer (" table " + <roa_table> + \0).
    const size_t length = (8 + strlen(bird_roa_table)) * sizeof (char);

    // Allocate buffer.
    bird_add_roa_table_arg = malloc(length);

    // Populate buffer.
    snprintf(bird_add_roa_table_arg, length, " table %s", bird_roa_table);
}

/**
 * Creates the buffer for the "add roa" command.
 */
void init_bird_command(void) {
    // Size of the buffer ("add roa " + <addr> + "/" + <minlen> + " max " +
    // <maxlen> + " as " + <asnum> + <bird_add_roa_table_cmd> + \0)
    bird_command_length = (
        8 + // "add roa "
        39 + // maxlength of IPv6 address
        1 + // "/"
        3 + // minimum length, "0" .. "128"
        5 + // " max "
        3 + // maximum length, "0" .. "128"
        4 + // " as "
        10 + // asnum, "0" .. "2^32 - 1" (max 10 chars)
        strlen(bird_add_roa_table_arg) + // length of fixed " table " + <table>
        1 // \0
    ) * sizeof (char);

    // Allocate buffer.
    bird_command = malloc(bird_command_length);
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
    struct pfx_table *table, const struct pfx_record record, const bool added
) {
    // IP address buffer.
    static char ip_addr_str[INET6_ADDRSTRLEN];

    // Buffer for BIRD response.
    static char bird_response[200];

    // Fetch IP address as string.
    ip_addr_to_str(&(record.prefix), ip_addr_str, sizeof(ip_addr_str));

    // Write BIRD command to buffer.
    if (
        snprintf(
            bird_command,
            bird_command_length,
            "%s roa %s/%d max %d as %d%s\n",
            added ? "add" : "delete",
            ip_addr_str,
            record.min_len,
            record.max_len,
            record.asn,
            bird_add_roa_table_arg
        )
        >= bird_command_length
    ) {
        syslog(LOG_ERR, "BIRD command too long.");
        return;
    }

    // Log the BIRD command and send it to the BIRD server.
    syslog(LOG_INFO, "To BIRD: %s", bird_command);
    write(bird_socket, bird_command, strlen(bird_command));

    // Fetch the answer and log.
    bird_response[read(bird_socket, bird_response, sizeof bird_response)] = 0;
    syslog(LOG_INFO, "From BIRD: %s", bird_response);
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

    // Setup BIRD ROA table command argument.
    if (config.bird_roa_table) {
        init_bird_add_roa_table_arg(config.bird_roa_table);
    }

    // Setup BIRD command buffer.
    init_bird_command();

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
                &rtr_callback
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

    // Cleanup memory.
    cleanup_bird_command();
    cleanup_bird_add_roa_table_arg();

    // Cleanup framework.
    cleanup();

    // Exit with success.
    return EXIT_SUCCESS;
}
