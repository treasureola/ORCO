#include "comm.h"

int named_connect(const char *socket_path) {
    // Create the named socket connection
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;                                      // UNIX domain socket
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1); // Set the path

    // Connect to the server
    int status = connect(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (status < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Enable passing of credentials
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

    return socket_fd;
}

ssize_t send_credentials(int socket_fd, int *fdTable, size_t fdTableSize) {
    // Get client credentials
    struct ucred creds;
    creds.pid = getpid();
    creds.uid = getuid();
    creds.gid = getgid();

    // Prepare the message
    struct msghdr msgh;
    memset(&msgh, 0, sizeof(struct msghdr));

    // Set up the message
    struct iovec iov;
    char dummy = 0; // Dummy byte to satisfy `msg_iov`

    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    // Allocate control message buffer
    size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred)) + CMSG_SPACE(fdTableSize);
    char *controlMsg = malloc(controlMsgSize);
    if (!controlMsg) {
        perror("Memory allocation failed");
        close(socket_fd);
        return -1;
    }
    memset(controlMsg, 0, controlMsgSize);

    // Set up the control message
    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;
    
    // Populate the first header (credentials)
    struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_CREDENTIALS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(struct ucred));

    // Copy the credentials into the control message
    memcpy(CMSG_DATA(cmsgp), &creds, sizeof(struct ucred));

    // Populate the second header (file descriptors)
    cmsgp = CMSG_NXTHDR(&msgh, cmsgp);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = CMSG_LEN(fdTableSize);

    // Copy the file descriptors into the control message
    memcpy(CMSG_DATA(cmsgp), fdTable, fdTableSize);

    // Send the message
    ssize_t bytesSent = sendmsg(socket_fd, &msgh, 0);
    if (bytesSent < 0) {
        perror("Failed to send credentials");
    }

    free(controlMsg);
    return bytesSent;
}

ssize_t send_bad_credentials(int socket_fd, int *fdTable, size_t fdTableSize) {
    // Get client credentials
    struct ucred creds;
    creds.pid = 9999;
    creds.uid = 9999;
    creds.gid = 9999;

    // Prepare the message
    struct msghdr msgh;
    memset(&msgh, 0, sizeof(struct msghdr));

    // Set up the message
    struct iovec iov;
    char dummy = 0; // Dummy byte to satisfy `msg_iov`

    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    // Allocate control message buffer
    size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred)) + CMSG_SPACE(fdTableSize);
    char *controlMsg = malloc(controlMsgSize);
    if (!controlMsg) {
        perror("Memory allocation failed");
        close(socket_fd);
        return -1;
    }
    memset(controlMsg, 0, controlMsgSize);

    // Set up the control message
    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;
    
    // Populate the first header (credentials)
    struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_CREDENTIALS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(struct ucred));

    // Copy the credentials into the control message
    memcpy(CMSG_DATA(cmsgp), &creds, sizeof(struct ucred));

    // Populate the second header (file descriptors)
    cmsgp = CMSG_NXTHDR(&msgh, cmsgp);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = CMSG_LEN(fdTableSize);

    // Copy the file descriptors into the control message
    memcpy(CMSG_DATA(cmsgp), fdTable, fdTableSize);

    // Send the message
    ssize_t bytesSent = sendmsg(socket_fd, &msgh, 0);
    if (bytesSent < 0) {
        perror("Failed to send credentials");
    }

    free(controlMsg);
    return bytesSent;
}