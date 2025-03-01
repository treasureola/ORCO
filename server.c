#include "comm.h"

void handle_client(int client_fd)
{
    sleep(1);
    printf("Server: Client %d connected\n", client_fd);

    size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred));
    char *controlMsg = malloc(controlMsgSize);

    struct msghdr msgh;
    memset(&msgh, 0, sizeof(struct msghdr));

    char dummy;
    struct iovec iov;
    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;

    ssize_t bytesReceived = recvmsg(client_fd, &msgh, 0);
    printf("Server: Received %ld bytes\n", bytesReceived);
    if (bytesReceived < 0)
    {
        perror("Failed to receive credentials");
        exit(EXIT_FAILURE);
    }

    struct cmsghdr *cmsgp;
    for (cmsgp = CMSG_FIRSTHDR(&msgh); cmsgp != NULL; cmsgp = CMSG_NXTHDR(&msgh, cmsgp))
    {
        printf("Server: Received control message type %d, level %d\n", cmsgp->cmsg_type, cmsgp->cmsg_level);

        if (cmsgp->cmsg_level == SOL_SOCKET && cmsgp->cmsg_type == SCM_CREDENTIALS)
        {
            struct ucred *creds = (struct ucred *)CMSG_DATA(cmsgp);
            printf("Received pid: %d\n", creds->pid);
            printf("Received uid: %d\n", creds->uid);
            printf("Received gid: %d\n", creds->gid);
        }
    }

    free(controlMsg);
    close(client_fd);
}

int main() {

    // Create the socket
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    unlink(SOCKET_PATH);

    struct sockaddr_un *addr = malloc(sizeof(struct sockaddr_un));
    memset(addr, 0, sizeof(struct sockaddr_un));

    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, SOCKET_PATH, sizeof(addr->sun_path) - 1);

    unlink(SOCKET_PATH);
    // Bind the socket
    if (bind(socket_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_un)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

    // Listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening\n");
    while (1)
    {
        // Accept the incoming connection
        int client_fd = accept(socket_fd, NULL, NULL);
        if (client_fd < 0)
        {
            continue;
        } else {
            // Handle the connection in a separate function or thread
            handle_client(client_fd);
        }
    }
}