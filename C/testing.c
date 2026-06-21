#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "parse_handler.h"
// #include "structures/queue.h"
// #include "structures/hashtable.h"
#include "structures/auxiliaryfunctions.h"

// creo una tabla hash de jobs activos y una cola para atender a los clientes en orden  


#define MAX_CONNECTIONS 100
#define MAX_EVENTS 1024
#define ANNOUNCE_INTERVAL 5
#define ANNOUNCE_TIMEOUT 15

#define UDP_PORT 5680
#define ERLANG_TCP_PORT 5678
#define NODE_TCP_PORT 5679

#define MAX_CPU 4
#define MAX_MEM 8192
#define MAX_GPU 1

int available_cpu = MAX_CPU;
int available_mem = MAX_MEM;
int available_gpu = MAX_GPU;

// Queue* queue_jobs = queue_create();
// Hash_table = hashTable_create(50, copy_request, comp_request, destroy_request, hash_request);
GList active_nodes = NULL;

int setup_udp_node_sock()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) perror("socket");
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    return sockfd;
}

struct sockaddr_in create_udp_broadcast_dest()
{
    struct sockaddr_in dest;

    dest.sin_family = AF_INET;
    dest.sin_port = htons(UDP_PORT);
    // dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    inet_pton(AF_INET, "26.227.8.255", &dest.sin_addr); // prueba radmin vpn

    return dest;
}

void broadcast_udp_node(int sockfd, struct sockaddr_in dest)
{
    char buffer[MAX_BUFF];
    printf("ANNOUNCE %d cpu:%d mem:%d gpu:%d\n", NODE_TCP_PORT, MAX_CPU, MAX_MEM, MAX_GPU);
    sprintf(buffer, "ANNOUNCE %d cpu:%d mem:%d gpu:%d", NODE_TCP_PORT, MAX_CPU, MAX_MEM, MAX_GPU);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&dest, sizeof(dest));
}

int handle_erlang_client(int client_conn_fd)
{
    PetitionInfo *info = parse_erlang_petition(client_conn_fd);

    //"info->structure" contiene la lista de requests en caso de request o un puntero a entero (jobid) en caso de status/release

    if(info->command == DISCONNECT)
    {
        printf("CLIENT WITH FD %d DISCONNECTED\n", client_conn_fd);
        // liberar recursos asociados a ese cliente
        
        return 0;
    }

    if(info->command == INVALID)
    {
        printf("INVALID REQUEST FROM FD %d\n", client_conn_fd);
        write(client_conn_fd, "INVALID_REQUEST\n", 16);
        return 1;
    }
    
    if(info->command == JOB_REQUEST)
    {
        printf("VALID REQUEST FROM FD %d\n", client_conn_fd);
        // for(gnode* node = info->structure ; node != NULL; node = node->next ){
        //     enqueue(queue_jobs,node->data,p_copy_request);
        // }
        write(client_conn_fd, "VALID_REQUEST\n", 14);
        return 1;
    }
}

// int handle_node_client(int client_conn_fd)
// {

// }

void handle_udp_announce(int udpfd)
{
    char buffer[MAX_BUFF];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    int n = recvfrom(udpfd, buffer, MAX_BUFF - 1, 0, (struct sockaddr *)&sender, &sender_len);

    if (n <= 0) return;

    buffer[n] = '\0';

    CNode temp;

    inet_ntop(AF_INET, &sender.sin_addr, temp.ip, sizeof(temp.ip));
    printf("[UDP FROM] %s\n", temp.ip);

    int ok = sscanf(buffer, "ANNOUNCE %d cpu:%d mem:%d gpu:%d", &temp.port, &temp.cpu, &temp.mem, &temp.gpu);

    if (ok != 4)
    {
        printf("[UDP ERROR] parse failed (ok=%d)\n", ok);
        return;
    }

    temp.last_seen = time(NULL);

    printf("[UDP PARSED] port=%d cpu=%d mem=%d gpu=%d time=%ld\n", temp.port, temp.cpu, temp.mem, temp.gpu, temp.last_seen);

    GNode *curr = active_nodes;

    while (curr != NULL)
    {
        CNode *node = curr->data;

        if (strcmp(node->ip, temp.ip) == 0)
        {
            printf("[UDP UPDATE] existing node %s updated\n", temp.ip);
            *node = temp;
            return;
        }

        curr = curr->next;
    }

    CNode *newNode = malloc(sizeof(CNode));
    if (!newNode)
        return;

    *newNode = temp;

    printf("[UDP INSERT] new node %s:%d added\n", temp.ip, temp.port);

    glist_addFront(active_nodes, newNode, identity_copy);
}

void remove_down_nodes(time_t now)
{

    GNode *curr = active_nodes;
    GNode *prev = NULL;

    while (curr != NULL)
    {
        CNode *node = curr->data;

        if (now - node->last_seen >= ANNOUNCE_TIMEOUT)
        {
            GNode *to_delete = curr;

            if (prev == NULL)
            {
                // borramos head
                active_nodes = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }

            curr = curr->next;

            free(node);
            free(to_delete);

            continue;
        }

        prev = curr;
        curr = curr->next;
    }
}

int setup_listening_erlang_sock()
{
    // creacion de sockets de escucha 
    int rc; // variable para chequear errores

    // Internet
    struct sockaddr_in sa;
    int lsockfd;
    int yes = 1;

    lsockfd = socket(AF_INET, SOCK_STREAM, 0); // socket por internet; AF_INET = ipv4
    if (lsockfd < 0) perror("socket");

    if (setsockopt(lsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == 1) // opcion para que no se quede tomado el puerto una vez finaliza el programa
        perror("setsockopt");

    sa.sin_family = AF_INET;
    sa.sin_port = htons(ERLANG_TCP_PORT); // puerto de prueba, puede ser cualquiera
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // ip local

    rc = bind(lsockfd, (struct sockaddr *)&sa, sizeof(sa));
    if (rc < 0) perror("bind");

    rc = listen(lsockfd, MAX_CONNECTIONS); // cola de hasta MAX_CONNECTIONS conexiones, es arbitrario
    if (rc < 0) perror("listen");

    return lsockfd;
}

int setup_listening_nodes_sock()
{
    // creacion de sockets de escucha 
    int rc; // variable para chequear errores

    // Internet
    struct sockaddr_in sa;
    int lsockfd;
    int yes = 1;

    lsockfd = socket(AF_INET, SOCK_STREAM, 0); // socket por internet; AF_INET = ipv4
    if (lsockfd < 0) perror("socket");

    if (setsockopt(lsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == 1) // opcion para que no se quede tomado el puerto una vez finaliza el programa
        perror("setsockopt");

    sa.sin_family = AF_INET;
    sa.sin_port = htons(NODE_TCP_PORT); // puerto de prueba, puede ser cualquiera
    sa.sin_addr.s_addr = htonl(INADDR_ANY); // ip cualquiera

    rc = bind(lsockfd, (struct sockaddr *)&sa, sizeof(sa));
    if (rc < 0) perror("bind");

    rc = listen(lsockfd, MAX_CONNECTIONS); // cola de hasta MAX_CONNECTIONS conexiones, es arbitrario
    if (rc < 0) perror("listen");

    return lsockfd;
}

void server_running_main(int epfd, struct epoll_event *events, int udpsockfd, struct sockaddr_in udp_dest)
{
    int rc; // variable para chequear errores

    time_t last_announce = time(NULL);
    
    while(1)
    {
        int n_events = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (n_events < 0) perror("epoll_wait");

        for (int i = 0; i < n_events; i++)
        {
            EventData *event = events[i].data.ptr;
            switch(event->type)
            {
                case LISTENER_ERLANG: // si el evento vino del socket de escucha de erlang
                {
                    int clientfd = accept(event->fd, NULL, NULL);

                    EventData *client = malloc(sizeof(EventData));
                    client->fd = clientfd;
                    client->type = CLIENT_ERLANG;

                    struct epoll_event ev;
                    ev.events = EPOLLIN; // EPOLLIN = hay datos para leer
                    ev.data.ptr = client;

                    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev); // agregamos a la conexion del cliente al epoll
                    if (rc < 0) perror("epoll_ctl");
                    break;
                }

                case LISTENER_NODE:
                {
                    int clientfd = accept(event->fd, NULL, NULL);

                    EventData *client = malloc(sizeof(EventData));
                    client->fd = clientfd;
                    client->type = CLIENT_NODE;

                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.ptr = client;

                    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
                    if (rc < 0) perror("epoll_ctl");
                    break;
                }

                case CLIENT_ERLANG:
                {
                    if (handle_erlang_client(event->fd) == 0)
                    {
                        close(event->fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, event->fd, NULL);
                        // liberar todos los jobs asociados
                        free(event);
                    }

                    break;
                }

                case CLIENT_NODE:
                {
                    // handle_node_client(event->fd);

                    close(event->fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, event->fd, NULL);

                    free(event);
                    break;
                }

                case UDP_NODE:
                {
                    handle_udp_announce(event->fd);
                    break;
                }
            }
        }

        time_t now = time(NULL);

        remove_down_nodes(now);

        if (now - last_announce >= ANNOUNCE_INTERVAL)
        {
            broadcast_udp_node(udpsockfd, udp_dest);
            last_announce = now;
        }
    }
}

int main()
{
    int rc; // variable para chequear errores

    int erlanglsockfd = setup_listening_erlang_sock(); // preparamos el socket de escucha de erlang
    int nodeslsockfd = setup_listening_nodes_sock(); // preparamos el socket de escucha de nodos
    int udpsockfd = setup_udp_node_sock(); // preparamos el socket udp para broadcast
    struct sockaddr_in udp_dest = create_udp_broadcast_dest(); // preparamos la direccion de destino del broadcast

    // empezamos epoll
    int epfd = epoll_create1(0); // creo objeto epoll
    if(epfd == -1) perror("epoll_create1");

    // socket erlang
    EventData *erlang_listener = malloc(sizeof(EventData));
    erlang_listener->fd = erlanglsockfd;
    erlang_listener->type = LISTENER_ERLANG;

    struct epoll_event ev_erlang;
    ev_erlang.events = EPOLLIN;
    ev_erlang.data.ptr = erlang_listener;

    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, erlanglsockfd, &ev_erlang);
    if (rc < 0) perror("epoll_ctl");

    // socket nodos
    EventData *nodes_listener = malloc(sizeof(EventData));
    nodes_listener->fd = nodeslsockfd;
    nodes_listener->type = LISTENER_NODE;

    struct epoll_event ev_nodes;
    ev_nodes.events = EPOLLIN;
    ev_nodes.data.ptr = nodes_listener;

    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, nodeslsockfd, &ev_nodes);
    if (rc < 0) perror("epoll_ctl");

    EventData *udp = malloc(sizeof(EventData));
    udp->fd = udpsockfd;
    udp->type = UDP_NODE;

    struct epoll_event ev_udp;
    ev_udp.events = EPOLLIN;
    ev_udp.data.ptr = udp;

    epoll_ctl(epfd, EPOLL_CTL_ADD, udpsockfd, &ev_udp);

    struct epoll_event events[MAX_EVENTS]; // estructura que nos va a almacenar la informacion de los eventos que se dispararon, hasta un maximo de MAX_EVENTS (arbitrario)

    broadcast_udp_node(udpsockfd, udp_dest); // hacemos el broadcast para que los nodos sepan donde esta el server

    int n_events = epoll_wait(epfd, events, MAX_EVENTS, 2000);

    if (n_events < 0) perror("epoll_wait");

    for (int i = 0; i < n_events; i++)
    {
        EventData *event = events[i].data.ptr;

        if (event->type == UDP_NODE)
            handle_udp_announce(event->fd);
    }

    server_running_main(epfd, events, udpsockfd, udp_dest);

    return 0;
}