#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtr.h"

// TCP address cannot be accessed from struct rtr_mgr_config, so store it here
// for later cleanup.
static struct tr_tcp_config *tcp_config = 0;

/**
 * Creates and returns an `rtr_mgr_config` structure from the specified
 * `rtr_mgr_group` structure.
 * @param groups
 * @return
 */
static struct rtr_mgr_config *rtr_create_mgr_config(
    rtr_mgr_group *groups, unsigned int count
) {
    // Initialize result.
    struct rtr_mgr_config *result = malloc(sizeof (struct rtr_mgr_config));
    memset(result, 0, sizeof (struct rtr_mgr_config));

    // Populate result.
    result->groups = groups;
    result->len = count;

    // Return result.
    return result;
}

/**
 * Creates and returns a `rtr_mgr_group` structure from the specified
 * `rtr_socket` structure array containing the specified number of elements.
 * @param rtr_socket
 * @param count
 * @return
 */
static rtr_mgr_group *rtr_create_mgr_group(
    struct rtr_socket *sockets, unsigned int count
) {
    // Create a prototype rtr_mgr_group to be able to retrieve its size.
    // TODO: Remove when rtr_mgr_group is not an anonymous struct anymore.
    rtr_mgr_group prototype;

    // Iterator.
    unsigned int i;

    // Initialize result.
    rtr_mgr_group *result = malloc(sizeof prototype); // TODO: see above
    memset(result, 0, sizeof prototype); // TODO: see above.

    // Populate result.
    result->sockets = malloc(count * sizeof (struct rtr_socket *));
    for (i = 0; i < count; i++)
        result->sockets[i] = sockets++;
    result->sockets_len = count;
    result->preference = 1;

    // Return result.
    return result;
}

/**
 * Creates and returns an `rtr_socket` structure from the specified `tr_socket`
 * structure.
 * @param socket
 * @return
 */
static struct rtr_socket *rtr_create_rtr_socket(struct tr_socket *socket) {
    // Create result.
    struct rtr_socket *result = malloc(sizeof (struct rtr_socket));
    memset(result, 0, sizeof (struct rtr_socket));

    // Populate result.
    result->tr_socket = socket;

    // Return result.
    return result;
}

/**
 * Creates and returns a new `tr_ssh_config` structure for an SSH connection to
 * the specified specified host and port, authenticated by the optional hostkey
 * path, with the specified user authenticated by the key pair.
 * @param host
 * @param port
 * @param server_hostkey_path
 * @param username
 * @param client_privkey_path
 * @param client_pubkey_path
 * @return
 */
static struct tr_ssh_config *rtr_create_ssh_config(
    const char *host,
    const unsigned int port,
    const char *server_hostkey_path,
    const char *username,
    const char *client_privkey_path,
    const char *client_pubkey_path
) {
    // Initialize result.
    struct tr_ssh_config *result = malloc(sizeof (struct tr_ssh_config));
    memset(result, 0, sizeof (struct tr_ssh_config));

    // Assign host, port and username (mandatory).
    result->host = strdup(host);
    result->port = port;
    result->username = strdup(username);

    // Assign key paths (optional).
    if (server_hostkey_path)
        result->server_hostkey_path = strdup(server_hostkey_path);
    if (client_privkey_path)
        result->client_privkey_path = strdup(client_privkey_path);
    if (client_pubkey_path)
        result->client_pubkey_path = strdup(client_pubkey_path);

    // Return result.
    return result;
}

/**
 * Creates, initializes (using `tr_ssh_init()`) and returns a `tr_socket`
 * structure from the specified `tr_ssh_config` structure for SSH connections to
 * an RTR server.
 * @param config
 * @return
 */
static struct tr_socket *rtr_create_ssh_socket(
    const struct tr_ssh_config *config
) {
    // Initialize result.
    struct tr_socket *result = malloc(sizeof (struct tr_socket));
    memset(result, 0, sizeof (struct tr_socket));

    // Initialize TCP socket.
    if (tr_ssh_init(config, result) != TR_SUCCESS) {
        fprintf(stderr, "Could not initialize TCP socket.");
        return 0;
    }

    // Return result.
    return result;
}

/**
 * Creates and returns a new `tr_tcp_config` structure from the specified host
 * and port.
 * @param host
 * @param port
 * @return
 */
static struct tr_tcp_config *rtr_create_tcp_config(
    const char *host, const char *port
) {
    // Initialize result.
    struct tr_tcp_config *result = malloc(sizeof (struct tr_tcp_config));
    memset(result, 0, sizeof (struct tr_tcp_config));

    // Populate result.
    result->host = strdup(host);
    result->port = strdup(port);

    // Store result in static variable until RTRLIB copies and frees the config.
    tcp_config = result;

    // Return result.
    return result;
}

/**
 * Creates, initializes (using `tr_tcp_init()`) and returns a `tr_socket`
 * structure from the specified `tr_tcp_config` structure for TCP connections to
 * an RTR server.
 * @param config
 * @return
 */
static struct tr_socket *rtr_create_tcp_socket(
    const struct tr_tcp_config *config
) {
    // Initialize result.
    struct tr_socket *result = malloc(sizeof (struct tr_socket));
    memset(result, 0, sizeof (struct tr_socket));

    // Initialize TCP socket.
    if (tr_tcp_init(config, result) != TR_SUCCESS) {
        fprintf(stderr, "Could not initialize TCP socket.");
        return 0;
    }

    // Return result.
    return result;
}

void rtr_close(struct rtr_mgr_config *rtr_mgr_config) {
    // Iterators.
    unsigned int group;
    unsigned int socket;

    // Stop the RTR manager.
    rtr_mgr_stop(rtr_mgr_config);

    // Free RTR manager internal structures.
    rtr_mgr_free(rtr_mgr_config);

    // Close and free all sockets from all groups.
    for (group = 0; group < rtr_mgr_config->len; group++) {
        for (
            socket = 0;
            socket < rtr_mgr_config->groups[group].sockets_len;
            socket++
        ) {
            tr_close(rtr_mgr_config->groups[group].sockets[socket]->tr_socket);
            tr_free(rtr_mgr_config->groups[group].sockets[socket]->tr_socket);
        }
        free(rtr_mgr_config->groups[group].sockets);
    }

    // Free groups and the config itself.
    free(rtr_mgr_config->groups);
    free(rtr_mgr_config);
    
    // Free tr_tcp_config structure. TODO: Delete when RTRLIB copies and frees.
    free(tcp_config);
    tcp_config = 0;
}

struct rtr_mgr_config *rtr_ssh_connect(
    const char *host, const char *port, const char *hostkey_file,
    const char *username, const char *privkey_file, const char *pubkey_file,
    const pfx_update_fp callback
) {
    // Create RTR manager config with the single server group.
    struct rtr_mgr_config *result = rtr_create_mgr_config(
        rtr_create_mgr_group(
            rtr_create_rtr_socket(
                rtr_create_ssh_socket(rtr_create_ssh_config(
                    host,
                    strtoul(port, 0, 10),
                    hostkey_file,
                    username,
                    privkey_file,
                    pubkey_file
                ))
            ),
            1
        ),
        1
    );

    // Initialize RTR manager and bail out on error.
    if (rtr_mgr_init(result, 240, 520, callback) == RTR_ERROR) {
        fprintf(stderr, "Error initializing RTR manager.");
        return 0;
    }

    // Start RTR manager and bail out on error.
    if (rtr_mgr_start(result) == RTR_ERROR) {
        fprintf(stderr, "Error starting RTR manager.");
        return 0;
    }

    // Return RTR manager config.
    return result;
}

struct rtr_mgr_config *rtr_tcp_connect(
    const char *host, const char *port, const pfx_update_fp callback
) {
    // Create RTR manager config with the single server group.
    struct rtr_mgr_config *result = rtr_create_mgr_config(
        rtr_create_mgr_group(
            rtr_create_rtr_socket(
                rtr_create_tcp_socket(rtr_create_tcp_config(host, port))
            ),
            1
        ),
        1
    );

    // Initialize RTR manager and bail out on error.
    if (rtr_mgr_init(result, 240, 520, callback) == RTR_ERROR) {
        fprintf(stderr, "Error initializing RTR manager.");
        return 0;
    }

    // Start RTR manager and bail out on error.
    if (rtr_mgr_start(result) == RTR_ERROR) {
        fprintf(stderr, "Error starting RTR manager.");
        return 0;
    }

    // Return RTR manager config.
    return result;
}
