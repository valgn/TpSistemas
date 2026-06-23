#include "../wrapper.h"

void remove_down_nodes(time_t now)
{
    GNode *curr = active_nodes;
    GNode *prev = NULL;

    while (curr)
    {
        CNode *node = (CNode *)curr->data;

        if (now - node->last_seen >= ANNOUNCE_TIMEOUT)
        {
            printf("[UDP] Down node Removed: %s\n", node->ip);

            GNode *dead = curr;
            if (prev) prev->next = curr->next;
            else       active_nodes = curr->next;

            curr = curr->next;
            free(node);
            free(dead);
            continue;
        }

        prev = curr;
        curr = curr->next;
    }
}

void handle_udp_announce(int udpfd)
{
    char buffer[MAX_BUFF];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    int n = recvfrom(udpfd, buffer, MAX_BUFF - 1, 0,
                     (struct sockaddr *)&sender, &sender_len);
    if (n <= 0) return;
    buffer[n] = '\0';

    CNode temp;
    inet_ntop(AF_INET, &sender.sin_addr, temp.ip, sizeof(temp.ip));

    int ok = sscanf(buffer, "ANNOUNCE %d cpu:%d mem:%d gpu:%d",
                    &temp.port, &temp.cpu, &temp.mem, &temp.gpu);
    if (ok != 4)
    {
        printf("[UDP ERROR] Failed Parse (ok=%d) msg='%s'\n", ok, buffer);
        return;
    }

    temp.last_seen = time(NULL);
    printf("[UDP] Node %s:%d cpu=%d mem=%d gpu=%d\n",
           temp.ip, temp.port, temp.cpu, temp.mem, temp.gpu);

    // We update if it already exists, insert if it's new
    GNode *curr = active_nodes;
    while (curr)
    {
        CNode *node = (CNode *)curr->data;
        if (strcmp(node->ip, temp.ip) == 0)
        {
            *node = temp;
            return;
        }
        curr = curr->next;
    }

    CNode *newNode = malloc(sizeof(CNode));
    if (!newNode) return;
    *newNode = temp;

    active_nodes = glist_addFront(active_nodes, newNode, identity_copy);
    printf("[UDP] New node: %s:%d\n", temp.ip, temp.port);
}


void broadcast_udp_node(int sockfd, struct sockaddr_in dest, int available_cpu, int available_mem, int available_gpu, int NODE_TCP_PORT)
{
    char buffer[MAX_BUFF];
    snprintf(buffer, sizeof(buffer),
             "ANNOUNCE %d cpu:%d mem:%d gpu:%d",
             NODE_TCP_PORT, available_cpu, available_mem, available_gpu);
    sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&dest, sizeof(dest));
    printf("[UDP] Broadcast sent: %s\n", buffer);
}


struct sockaddr_in create_udp_broadcast_dest(int UDP_PORT)
{
    struct sockaddr_in dest;
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(UDP_PORT);
    dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    return dest;
}

int setup_udp_node_sock(int UDP_PORT)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket udp"); return -1; }

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(UDP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        perror("bind udp");

    return sockfd;
}
