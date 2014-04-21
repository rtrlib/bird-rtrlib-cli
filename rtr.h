#ifndef BIRD_RTRLIB_CLI__RTR_H
#define	BIRD_RTRLIB_CLI__RTR_H

#include <rtrlib/rtrlib.h>

/**
 * Connects to the RTR server at the specified address and port using the
 * specified callback functions for RTR updates and returns the corresponding
 * `rtr_mgr_config` structure. Returns a null pointer on error.
 * @param host
 * @param port
 * @param callback
 * @return 
 */
struct rtr_mgr_config *rtr_connect(
    const char *, const char *, const pfx_update_fp
);

/**
 * Stops the RTR manager with the specified config and frees all resources
 * allocated by `rtr_connect()`.
 * @param rtr_config
 */
void rtr_close(struct rtr_mgr_config *);

#endif
