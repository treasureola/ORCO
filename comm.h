#ifndef H_ORCO_COMMON
#define H_ORCO_COMMON
#define _GNU_SOURCE
#include <sys/socket.h> // For socket(), sendmsg(), recvmsg(), and SCM_CREDENTIALS
#include <sys/types.h>  // For pid_t, uid_t, etc.
#include <sys/un.h>
#include <unistd.h>     // For getpid(), etc.
#include <stdio.h>      // For perror(), printf(), etc.
#include <stdlib.h>     // For exit(), etc.
#include <string.h>
#include <poll.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/socket"
#define MAX_EVENTS 10
#define MAX_FILES 10

#define DEBUG_MODE
#ifdef DEBUG_MODE
    #define DEBUG_PRINT(fmt, ...) fprintf(stderr, "DEBUG:   " fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) // Do nothing when DEBUG_MODE is not defined
#endif

#endif