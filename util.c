#include <stddef.h>
#include <syslog.h>

void cleanup(void) {
    closelog();
}

void init(void) {
    openlog(NULL, LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);
}
