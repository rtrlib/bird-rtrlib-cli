#ifndef BIRD_RTRLIB_CLI__RTR_H
#define	BIRD_RTRLIB_CLI__RTR_H

#include <rtrlib/rtrlib.h>

/**
 * Connects to the specified SSH server using the specified callback function
 * for RTR updates and returns the corresponding `rtr_mgr_config` structure.
 * Returns a null pointer on error.
 * @param host host name of the SSH server.
 * @param port port number of the SSH server in string representation.
 * @param hostkey_file path to a file containing the host key of the specified
 * server. If set to a null pointer, the default known_hosts file is used.
 * @param username name of the user to be authenticated with the SSH server.
 * @param privkey_file path to a file containing the private key to be used for
 * authentication with the SSH server. If set to a null pointer, the user's
 * default identity is used.
 * @param pubkey_file path to a file containing the public key to be used for
 * authentication with the SSH server. If set to a null pointer, the user's
 * default public key is used.
 * @param callback
 * @return
 */
struct rtr_mgr_config *rtr_ssh_connect(
    const char *, const char *, const char *, const char *, const char *,
    const char *, const pfx_update_fp
);

/**
 * Connects to the RTR server at the specified address and port with plain TCP
 * using the specified callback function for RTR updates and returns the
 * corresponding `rtr_mgr_config` structure. Returns a null pointer on error.
 * @param host
 * @param port
 * @param callback
 * @return
 */
struct rtr_mgr_config *rtr_tcp_connect(
    const char *, const char *, const pfx_update_fp
);

/**
 * Stops the RTR manager with the specified config and frees all resources
 * allocated by `rtr_connect()`.
 * @param rtr_config
 */
void rtr_close(struct rtr_mgr_config *);

#endif
