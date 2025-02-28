
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/socket"

struct ucred
{
    pid_t pid;
    uid_t uid;
    gid_t gid;
};
