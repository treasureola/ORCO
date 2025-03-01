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

    struct sockaddr_un *addr = malloc(sizeof(struct sockaddr_un));
    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, SOCKET_PATH, sizeof(addr->sun_path) - 1);

    printf("Client connecting\n");
    // Connect to the server
    int status = connect(socket_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_un));
    if (status < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server\n");

    while (1){}    


    // // Send credentials to the server
    // struct ucred creds;
    // creds.pid = pid;
    // creds.uid = uid;
    // creds.gid = gid;
    // (void)creds;

    // struct msghdr msgh = {0};
    
    // size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred));
    // char *controlMsg = malloc(controlMsgSize);
    // msgh.msg_control = controlMsg;
    // msgh.msg_controllen = controlMsgSize;

    // struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgh);
    // cmsg->cmsg_level = SOL_SOCKET;
    // cmsg->cmsg_type = SCM_CREDENTIALS;

    // Close the socket
    close(socket_fd);

    return 0;
}