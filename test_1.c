// To test sending fake credentials to the server

#include "comm.h"

int main(){
    pid_t pid = getpid();
    uid_t uid = getuid();
    gid_t gid = getgid();
    
    printf("Client pid: %d\n", pid);
    printf("Client uid: %d\n", uid);
    printf("Client gid: %d\n", gid);

    // Create the socket
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX; // Change addr-> to addr.
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to the server
    int status = connect(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (status < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    // Send credentials to the server : these are fake credentials for the purpose of testing
    struct ucred creds;
    creds.pid = 9999;
    creds.uid = 9999;
    creds.gid = 9999;

    struct msghdr msgh;
    memset(&msgh, 0, sizeof(struct msghdr));

    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

    struct iovec iov;
    char dummy = 0; // Dummy byte to satisfy `msg_iov`

    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    // Allocate control message buffer
    size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred));
    char *controlMsg = malloc(controlMsgSize);
    if (!controlMsg)
    {
        perror("Memory allocation failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    memset(controlMsg, 0, controlMsgSize);

    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;
    
    struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_CREDENTIALS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(struct ucred));

    memcpy(CMSG_DATA(cmsgp), &creds, sizeof(struct ucred));

    ssize_t bytesSent = sendmsg(socket_fd, &msgh, 0);
    printf("Client: Sent %ld bytes\n", bytesSent);
    if (bytesSent < 0) {
        perror("Failed to send credentials");
        exit(EXIT_FAILURE);
    }

    // Close the socket
    close(socket_fd);

    free(controlMsg);

    return 0;
}