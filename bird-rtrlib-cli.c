#include <stdlib.h>
#include <string.h>

#include "config.h"

int main(int argc, char *argv[]) {
    // Configuration.
    struct config config;
    
    // Socket to BIRD.
    int bird_socket;
    
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
    
    // TODO: RTRLIB integration.
    
    // TODO: Server loop.
    
    // Clean up and exit.
    close(bird_socket);
    return EXIT_SUCCESS;
}
