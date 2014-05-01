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
