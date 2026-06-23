#include "../wrapper.h"

int setup_listening_nodes_sock(int node_tcp_port)
{
    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lsockfd < 0) { perror("socket nodes"); return -1; }

    int yes = 1;
    setsockopt(lsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in sa;
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(node_tcp_port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);         /* public network */

    if (bind(lsockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        perror("bind nodes");
    if (listen(lsockfd, MAX_CONNECTIONS) < 0)
        perror("listen nodes");

    return lsockfd;
}

int setup_listening_erlang_sock(int erlang_tcp_port)
{
    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lsockfd < 0) { perror("socket erlang"); return -1; }

    set_nonblocking(lsockfd);

    int yes = 1;
    setsockopt(lsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in sa;
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(erlang_tcp_port);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   /* only localhost */

    if (bind(lsockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        perror("bind erlang");
    if (listen(lsockfd, MAX_CONNECTIONS) < 0)
        perror("listen erlang");

    return lsockfd;
}

int connect_to_node(const char *ip, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("[NETWORK] Error in creating socket descriptor");
        return -1;
    }

    /* Use blocking connect to simplify remote connection handling.
       Non-blocking connect returns -1 with EINPROGRESS and the current
       code did not handle that case, causing spurious failures. */

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &remote_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "[NETWORK] IP Address invalid or not supported: %s\n", ip);
        close(fd);
        return -1;
    }

    // Tries to connect to the remote node (blocking).
    if (connect(fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
    {
        perror("[NETWORK] connect to node failed");
        close(fd);
        return -1;
    }

    return fd;
}

int set_nonblocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}