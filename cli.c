#include <argp.h>
#include <stddef.h>
#include <string.h>

#include "cli.h"

// Parser function for argp_parse().
static error_t argp_parser(int key, char *arg, struct argp_state *state) {
    // Shortcut to config object passed to argp_parse().
    struct config *config = state->input;

    // Process command line argument.
    switch (key) {
        case 'b':
            // Process BIRD socket path.
            config->bird_socket_path = arg;
            break;
        case 'r':
            config->rtr_host = strtok(arg, ":");
            config->rtr_port = strtok(0, ":");
            break;
        default:
            // Process unknown argument.
            return ARGP_ERR_UNKNOWN;
    }

    // Return success.
    return 0;
}

// Parses the specified command line arguments into the program config.
int parse_cli(int argc, char **argv, struct config *config) {
    // Command line options definition.
    const struct argp_option argp_options[] = {
        {"bird", 'b', "BIRD_SOCKET_PATH", 0, "Path to the BIRD control socket", 0},
        {"rtr", 'r', "RTR_ADDRESS", 0, "Address of the RTR server", 0},
        {0}
    };

    // argp structure to be passed to argp_parse().
    const struct argp argp = {
        argp_options,
        &argp_parser,
        NULL,
        "RTRLIB <-> BIRD interface",
        NULL,
        NULL,
        NULL
    };

    // Parse command line. Exits on errors.
    argp_parse(&argp, argc, argv, 0, NULL, config);

    // Return success.
    return 1;
}
