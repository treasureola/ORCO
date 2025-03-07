#define SERVER_TESTS
#define TEST(func) printf("[[ %s ]]\n", #func); if (!func()) {printf("[[ %s passed ]]\n", #func);} else {printf("[[ %s failed ]]\n", #func);}
#define EQRF(arg, against) if((long)arg == against) {return -1;}
#define NEQRF(arg, against) if((long)arg != against) {return -1;}
#define LTRF(arg, against) if((long)arg < against) {return -1;}
#define TODO printf("Test Not Yet Implemented\n"); return 0;
#include "server.c"
#include "server_test_helpers.c"
 
int test_socket_setup() {
    struct pollfd *fds = setup_socket(SOCKET_PATH);
    EQRF(fds, EXIT_FAILURE);

    return 0;
}

int test_accept_new_connection() {
    struct pollfd *fds = setup_socket(SOCKET_PATH);
    int listen_sock = fds[0].fd;

    pid_t pid = fork();
    if (!pid) {
        named_connect(SOCKET_PATH);
        _exit(0);
    }

    LTRF(poll(fds, 1, 3000), 1);
    int ret = accept_new_connection(listen_sock, fds, NULL);
    EQRF(ret, EXIT_FAILURE);

    return 0;
}

int test_verify_client_success() {
    struct pollfd *fds = setup_socket(SOCKET_PATH);
    int listen_sock = fds[0].fd;

    pid_t pid = fork();
    if (!pid) {
        int server_fd = named_connect(SOCKET_PATH);
        int fdTableSize = 1*sizeof(int);
        int *fdTable = malloc(fdTableSize);
        fdTable[0] = 1;
        send_credentials(server_fd, fdTable, fdTableSize);
        _exit(0);
    }

    LTRF(poll(fds, 1, 3000), 1);
    int ret = accept_new_connection(listen_sock, fds, NULL);
    
    EQRF(ret, 1);
    EQRF(ret, -2);

    return 0;
}

int test_verify_client_failure() {
    struct pollfd *fds = setup_socket(SOCKET_PATH);
    int listen_sock = fds[0].fd;

    pid_t pid = fork();
    if (!pid) {
        int server_fd = named_connect(SOCKET_PATH);
        int fdTableSize = 1*sizeof(int);
        int *fdTable = malloc(fdTableSize);
        fdTable[0] = 1;
        send_bad_credentials(server_fd, fdTable, fdTableSize);
        _exit(0);
    }

    LTRF(poll(fds, 1, 3000), 1);
    int ret = accept_new_connection(listen_sock, fds, NULL);
    
    NEQRF(ret, -1);

    return 0;
}

int test_handle_client_connection() {
    TODO
}

int test_handle_request() {
    TODO
}

int main() {
    TEST(test_socket_setup);
    TEST(test_accept_new_connection);
    TEST(test_verify_client_success);
    TEST(test_verify_client_failure);
    return 0;
}