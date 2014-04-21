#ifndef BIRD_RTRLIB_CLI__CONFIG_H
#define	BIRD_RTRLIB_CLI__CONFIG_H

/**
 * Application configuration structure.
 */
struct config {
    char *bird_socket_path;
    char *rtr_host;
    char *rtr_port;
};

/**
 * Checks the specified application configuration for errors.
 * @param 
 * @return 
 */
int config_check(const struct config *);

/**
 * Initializes the specified application configuration.
 * @param 
 */
void config_init(struct config *);

#endif
