#ifndef H_ORCO_COMMON
#define H_ORCO_COMMON
#define _GNU_SOURCE
#define TRUE 1
#define FALSE 0
#include <sys/socket.h> // For socket(), sendmsg(), recvmsg(), and SCM_CREDENTIALS
#include <sys/types.h>  // For pid_t, uid_t, etc.
#include <sys/un.h>
#include <unistd.h>     // For getpid(), etc.
#include <stdio.h>      // For perror(), printf(), etc.
#include <stdlib.h>     // For exit(), etc.
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SOCKET_PATH "/tmp/socket"
#define MAX_EVENTS 10
#define MAX_FILES 10
#define MAX_CLIENTS 10

typedef struct Client
{
    int socket;
    // struct sockaddr_in address;
    int fd;
} Client;


//#define DEBUG_MODE
#ifdef DEBUG_MODE
    #define DEBUG_PRINT(fmt, ...) fprintf(stderr, "DEBUG:   " fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) // Do nothing when DEBUG_MODE is not defined
#endif

typedef struct {
    int acces_code; //the access code of the user
    char action[50]; // To read or write
    char filename[128];  // file the user wnats to read or wort to
    char write_string[1024]; // IF write, the data the user wants to worite to the file
    int read_bytes; //If read, the number of bytes the user wants to read
}Request;

#endif


