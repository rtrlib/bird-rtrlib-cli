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
 * written by smlng and Mehmet Ceyran, in cooperation with:
 * CST group, Freie Universitaet Berlin
 * Website: https://github.com/rtrlib/bird-rtrlib-cli
 */

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bird.h"
#include "cli.h"
#include "config.h"
#include "rtr.h"
#include <rtrlib/rtrlib.h>

#define CMD_EXIT "exit"
#define BIRD_RSP_SIZE  (200)
// Socket to BIRD.
static int bird_socket = -1;
// Buffer for BIRD commands.
static char *bird_command = 0;
// Length of buffer for BIRD commands.
static size_t bird_command_length = -1;
// "add roa" BIRD command "table" part. Defaults to an empty string and becomes
// "table " + config->bird_roa_table if provided.
static char *bird_add_roa_table_arg = "";
// Main configuration.
static struct config config;
// for daemon loop
static int time_to_die = 0;
// fopr pidfile
static int pid_fd = -1;

/**
 * Handle SIGPIPE on bird_socket, write error log entry
 */
void sigpipe_handler(int signum)
{
    if (config.quiet != true) 
        syslog(LOG_ERR, "Caught SIGPIPE %d.", signum);
}

void sigkill_handler(int signum)
{
	syslog(LOG_INFO, "Got kill signal, cleaning up and exiting.");
	/* Unlock and close lockfile */
	if (pid_fd != -1) {
		lockf(pid_fd, F_ULOCK, 0);
		close(pid_fd);
	}
	/* Try to delete lockfile */
	if (config.pidfile != NULL) {
		unlink(config.pidfile);
	}
	time_to_die++;
}

void create_pidfile()
{
    char str[256];
    pid_fd = open(config.pidfile, O_RDWR|O_CREAT, 0640);
    if (pid_fd < 0) 
	exit(EXIT_FAILURE);
    if (lockf(pid_fd, F_TLOCK, 0) < 0) 
        exit(EXIT_FAILURE);
    sprintf(str,"%d\n", getpid());
    write(pid_fd, str, strlen(str));
}

/**
 * Performs cleanup on resources allocated by `init()`.
 */
void cleanup(void)
{
    closelog();
}

/**
 * Frees memory allocated with the " table <bird_roa_table>" clause for the
 * "add roa" BIRD command.
 */
void cleanup_bird_add_roa_table_arg(void)
{
    // free buffer only, if not empty - i.e., mem was malloc'd
    if (strcmp(bird_add_roa_table_arg, "") != 0)
        free(bird_add_roa_table_arg);
}

/**
 * Frees memory allocated with the BIRD command buffer.
 */
void cleanup_bird_command(void)
{
    if (bird_command) {
        free(bird_command);
        bird_command = 0;
        bird_command_length = -1;
    }
}

/**
 * Initializes the application prerequisites.
 */
void init(void)
{
    openlog(NULL, LOG_PERROR | LOG_CONS | LOG_PID, LOG_DAEMON);
}

/**
 * Creates and populates the "add roa" command's "table" argument buffer.
 * @param bird_roa_table
 */
void init_bird_add_roa_table_arg(char *bird_roa_table)
{
    // Size of the buffer (" table " + <roa_table> + \0).
    const size_t length = (8 + strlen(bird_roa_table)) * sizeof (char);
    // Allocate buffer.
    bird_add_roa_table_arg = malloc(length);
    // Populate buffer.
    snprintf(bird_add_roa_table_arg, length, " table %s", bird_roa_table);
}

/**
 * Creates the buffer for the "add roa" command.
 */
void init_bird_command(void)
{
    // Size of the buffer ("add roa " + <addr> + "/" + <minlen> + " max " +
    // <maxlen> + " as " + <asnum> + <bird_add_roa_table_cmd> + \0)
    bird_command_length = (
        8 + // "add roa "
        39 + // maxlength of IPv6 address
        1 + // "/"
        3 + // minimum length, "0" .. "128"
        5 + // " max "
        3 + // maximum length, "0" .. "128"
        4 + // " as "
        10 + // asnum, "0" .. "2^32 - 1" (max 10 chars)
        strlen(bird_add_roa_table_arg) + // length of fixed " table " + <table>
        1 // \0
    ) * sizeof (char);
    // Allocate buffer.
    bird_command = malloc(bird_command_length);
}

/**
 * Callback function for RTRLib that receives PFX records, translates them to
 * BIRD `add roa` commands, sends them to the BIRD server and fetches the
 * answer.
 * @param table
 * @param record
 * @param added
 */
static void pfx_update_callback(struct pfx_table *table,
                                const struct pfx_record record,
                                const bool added)
{
    // IP address buffer.
    static char ip_addr_str[INET6_ADDRSTRLEN];
    // Buffer for BIRD response.
    static char bird_response[BIRD_RSP_SIZE];
    // Fetch IP address as string.
    lrtr_ip_addr_to_str(&(record.prefix), ip_addr_str, sizeof(ip_addr_str));
    // Crude way of filtering out sending Bird the wrong kind of updates
    if (config.ip_version)
    {
        int prefix_allowed = 0;
	// Is this an IPv4 prefix?
        if ((strchr(ip_addr_str,'.') != NULL) && (strchr(config.ip_version, '4') != NULL) )  
	    prefix_allowed++;
        if ((strchr(ip_addr_str,':') != NULL) && (strchr(config.ip_version, '6') != NULL) )  
            prefix_allowed++;
        if (prefix_allowed == 0) 
    	    return;
    }
    // Write BIRD command to buffer.
    if (
        snprintf(
            bird_command,
            bird_command_length,
            "%s roa %s/%u max %u as %u%s\n",
            added ? "add" : "delete",
            ip_addr_str,
            record.min_len,
            record.max_len,
            record.asn,
            bird_add_roa_table_arg
        )
        >= bird_command_length
    ) {
        syslog(LOG_ERR, "BIRD command too long.");
        return;
    }
    // Log the BIRD command and send it to the BIRD server.
    if (config.quiet != true) 
        syslog(LOG_INFO, "To BIRD: %s", bird_command);
    // reconnect bird_socket on SIGPIPE error, and resend BIRD command
    while ((write(bird_socket, bird_command, strlen(bird_command)) < 0) &&
           (errno == EPIPE)) {
        if (config.quiet != true) 
            syslog(LOG_ERR, "BIRD socket send failed, try reconnect!");
        close(bird_socket);
        bird_socket = bird_connect(config.bird_socket_path);
    }
    // Fetch BIRD answer, reconnect bird_socket on SIGPIPE while receive
    int size = -1;
    while (((size = read(bird_socket, bird_response, BIRD_RSP_SIZE-1)) <0) &&
           (errno == EPIPE)){
        syslog(LOG_ERR, "BIRD socket recv failed, try reconnect!");
        close(bird_socket);
        bird_socket = bird_connect(config.bird_socket_path);
    }
    // log answer, if any valid response
    if (size > 0) {
        bird_response[size] = 0;
	// Any bird response that doesn't start with 0000 is bad
	if ((strstr(bird_response, "0000") == NULL) && (strstr(bird_response, "0001") == NULL))
		syslog(LOG_ERR,"Bird command %s resulted in: %s\n", bird_command, bird_response);
//	else
//		fprintf (stdout,"%s %s/%d \t", added ? "add" : "del", ip_addr_str, record.min_len);
        if (config.quiet != true) 
            syslog(LOG_INFO, "From BIRD: %s", bird_response);
    }
}

/**
 * Entry point to the BIRD RTRLib integration application.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{

    // Buffer for commands and its length.
    char *command = 0;
    size_t command_len = 0;
    // Initialize variables.
    config_init(&config);
    // Initialize framework.
    init();
    // Parse CLI arguments into config and bail out on error.
    if (!parse_cli(argc, argv, &config)) {
        cleanup();
        fprintf(stderr, "Invalid command line parameter!\n");
        return EXIT_FAILURE;
    }
    // Check config.
    if (config_check(&config)) {
        cleanup();
        fprintf(stderr, "Invalid configuration parameters!\n");
        return EXIT_FAILURE;
    }
    pid_t process_id = 0;
    pid_t sid = 0;

    // Launch daemon if configured
    if (config.daemon == true)
    {
	    process_id = fork ();
	    // fork failed
	    if (process_id < 0)
	    {
		    fprintf(stderr,"fork failed!\n");
		    exit(1);
	    }
	    // Parent process, end it
	    if (process_id > 0) 
		    exit(0);

	    // We're now in the child process
	    if (config.pidfile != NULL) 
		    create_pidfile();
	    syslog(LOG_INFO, "initiated and running.");
	    sid = setsid();
	    if (sid < 0)
		    exit(1);
	    chdir ("/");
    }

    // Setup BIRD ROA table command argument.
    if (config.bird_roa_table) {
        init_bird_add_roa_table_arg(config.bird_roa_table);
    }
    // Setup BIRD command buffer.
    init_bird_command();
    // Try to connect to BIRD and bail out on failure.
    bird_socket = bird_connect(config.bird_socket_path);
    if (bird_socket == -1) {
        cleanup();
        syslog(LOG_ERR, "Failed to connect to BIRD socket!\n");
        return EXIT_FAILURE;
    }

    struct tr_socket tr_sock;
    struct tr_tcp_config *tcp_config;
    struct tr_ssh_config *ssh_config;
    // Try to connect to the RTR server depending on requested connection type.
    switch (config.rtr_connection_type) {
        case tcp:
            tcp_config = rtr_create_tcp_config(
                config.rtr_host, config.rtr_port, config.rtr_bind_addr);
            tr_tcp_init(tcp_config, &tr_sock);
            break;
        case ssh:
            ssh_config = rtr_create_ssh_config(
                config.rtr_host, config.rtr_port, config.rtr_bind_addr,
                config.rtr_ssh_hostkey_file, config.rtr_ssh_username,
                config.rtr_ssh_privkey_file);
            tr_ssh_init(ssh_config, &tr_sock);
            break;
        default:
            cleanup();
            syslog(LOG_ERR, "Invalid connection type, use tcp or ssh!\n");
            return EXIT_FAILURE;
    }

    struct rtr_socket rtr;
    struct rtr_mgr_config *conf;
    struct rtr_mgr_group groups[1];
    // init rtr_socket and groups
    rtr.tr_socket = &tr_sock;
    groups[0].sockets_len = 1;
    groups[0].sockets = malloc(1 * sizeof(rtr));
    groups[0].sockets[0] = &rtr;
    groups[0].preference = 1;
    // init rtr_mgr
    int ret = rtr_mgr_init(&conf, groups, 1, 30, 600, 600,
                           pfx_update_callback, NULL, NULL, NULL);
    // check for init errors
    if (ret == RTR_ERROR) {
        syslog(LOG_ERR, "Error in rtr_mgr_init!\n");
        return EXIT_FAILURE;
    }
    else if (ret == RTR_INVALID_PARAM) {
        syslog(LOG_ERR, "Invalid params passed to rtr_mgr_init\n");
        return EXIT_FAILURE;
    }
    // check if rtr_mgr config valid
    if (!conf) {
        syslog(LOG_ERR, "No config for rtr manager!\n");
        return EXIT_FAILURE;
    }
    // set handler for SIGPIPE
    signal(SIGPIPE, sigpipe_handler);
    // set handler for SIGKILL and SIGTERM
    signal(SIGKILL, sigkill_handler);
    signal(SIGTERM, sigkill_handler);
    // start rtr_mgr
    rtr_mgr_start(conf);

    if (config.daemon == true)
    {
	    close(STDIN_FILENO);
	    close(STDOUT_FILENO);
	    close(STDERR_FILENO);
	    // Child loop
	    while(time_to_die == 0)
		    sleep(1);
    }
    else
    {
	    fprintf(stdout, "bird-rtrlib-cli connected to %s:%s ready for IP versions %s.\nType 'exit' to clean up and quit.\n",
		    config.rtr_host,
		    config.rtr_port,
		    config.ip_version ? config.ip_version : "all"
            );
	    // CLI loop. Read commands from stdin.
	    while (getline(&command, &command_len, stdin) != -1) {
	        if (strncmp(command, CMD_EXIT, strlen(CMD_EXIT)) == 0)
	            break;
    	    }
    }
    // Clean up RTRLIB memory.
    rtr_mgr_stop(conf);
    rtr_mgr_free(conf);
    free(groups[0].sockets);
    // Close BIRD socket.
    close(bird_socket);
    // Cleanup memory.
    cleanup_bird_command();
    cleanup_bird_add_roa_table_arg();
    // Cleanup framework.
    cleanup();
    // Exit with success.
    return EXIT_SUCCESS;
}
