#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/un.h>

/**
 * 
 */
int bird_connect(const char *socket_path) {
    // Result value containing the socket to the BIRD.
    int bird_socket = -1;
    
    // Socket address to the BIRD.
    struct sockaddr_un addr;

    // Check socket path length.
    if (strlen(socket_path) >= sizeof addr.sun_path) {
        syslog(LOG_EMERG, "Socket path too long");
        return -1;
    }

    // Create socket and bail out on error.
    bird_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (bird_socket < 0) {
        syslog(LOG_EMERG, "Socket creation error: %m");
        return -1;
    }

    // Create socket address.
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    
    // Try to connect to BIRD.
    if (connect(bird_socket, (struct sockaddr *) &addr, sizeof addr) == -1) {
        syslog(LOG_EMERG, "BIRD connection to %s failed: %m", socket_path);
        close(bird_socket);
        return -1;
    }
    
    // TODO: Async connection.
    
    // Return socket.
    return bird_socket;
}
