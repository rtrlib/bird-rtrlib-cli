/* 
 * File:   config.h
 * Author: Mehmet
 *
 * Created on 30. März 2014, 16:38
 */

#ifndef CONFIG_H
#define	CONFIG_H

#define BIRD_SOCKET_PATH_DEFAULT "/usr/local/var/run/bird.ctl"

struct config {
    char *bird_socket_path;
};

#endif
