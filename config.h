#ifndef BIRD_RTRLIB_CLI__CONFIG_H
#define	BIRD_RTRLIB_CLI__CONFIG_H

/// Specifies a type of server connection to be used.
enum connection_type {
    // Plain TCP connection
    tcp,
    
    // SSH connection
    ssh
};

/**
 * Application configuration structure.
 */
struct config {
    char *bird_socket_path;
    enum connection_type rtr_connection_type;
    char *rtr_host;
    char *rtr_port;
    char *rtr_ssh_username;
    char *rtr_ssh_hostkey_file;
    char *rtr_ssh_privkey_file;
    char *rtr_ssh_pubkey_file;
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
