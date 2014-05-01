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

#include <stdio.h>
#include <string.h>

#include "config.h"

/**
 * Checks the specified application config for errors. Returns `1` if it has no
 * errors, or `0` otherwise.
 * @param config
 * @return
 */
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

    // Checks to be done for SSH connections.
    if (config->rtr_connection_type == ssh) {
        // Check SSH username availability.
        if (!config->rtr_ssh_username) {
            fprintf(stderr, "Missing SSH username.");
            return 0;
        }
    }

    // Return success.
    return 1;
}

/**
 * Initializes the specified application configuration.
 * @param config
 */
void config_init(struct config *config) {
    // Reset memory.
    memset(config, 0, sizeof (struct config));

    // Default connection type is TCP.
    config->rtr_connection_type = tcp;
}
