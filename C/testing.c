#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <stdlib.h>

#include "parse_handler.h"

#define MAX_CONNECTIONS 100
#define MAX_EVENTS 1024

#define MAX_CPU 4
#define MAX_MEM 8192
#define MAX_GPU 1

int available_cpu = MAX_CPU;
int available_mem = MAX_MEM;
int available_gpu = MAX_GPU;

int handle_erlang_client(int client_conn_fd)
{
    PetitionInfo *info = parse_erlang_petition(client_conn_fd);

    //"info->structure" contiene la lista de requests en caso de request o un puntero a entero (jobid) en caso de status/release

    if(info->command == DISCONNECT)
    {
        printf("CLIENT WITH FD %d DISCONNECTED\n", client_conn_fd);
        return 0;
    }

    if(info->command == INVALID)
    {
        printf("INVALID REQUEST FROM FD %d\n", client_conn_fd);
        write(client_conn_fd, "INVALID_REQUEST\n", 16);
        return 1;
    }
    else
    {
        printf("VALID REQUEST FROM FD %d\n", client_conn_fd);
        write(client_conn_fd, "VALID_REQUEST\n", 14);
        return 1;
    }
}

// int handle_node_client(int client_conn_fd)
// {

// }

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
    sa.sin_port = htons(5678); // puerto de prueba, puede ser cualquiera
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
    sa.sin_port = htons(5679); // puerto de prueba, puede ser cualquiera
    sa.sin_addr.s_addr = htonl(INADDR_ANY); // ip cualquiera

    rc = bind(lsockfd, (struct sockaddr *)&sa, sizeof(sa));
    if (rc < 0) perror("bind");

    rc = listen(lsockfd, MAX_CONNECTIONS); // cola de hasta MAX_CONNECTIONS conexiones, es arbitrario
    if (rc < 0) perror("listen");

    return lsockfd;
}

void server_running_main(int epfd, struct epoll_event *events)
{
    int rc; // variable para chequear errores
    while(1)
    {
        int n_events = epoll_wait(epfd, events, MAX_EVENTS, -1);
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
            }
        }
    }
}

int main()
{
    int rc; // variable para chequear errores

    int erlanglsockfd = setup_listening_erlang_sock(); // preparamos el socket de escucha de erlang
    int nodeslsockfd = setup_listening_nodes_sock(); // preparamos el socket de escucha de nodos

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

    struct epoll_event events[MAX_EVENTS]; // estructura que nos va a almacenar la informacion de los eventos que se dispararon, hasta un maximo de MAX_EVENTS (arbitrario)

    server_running_main(epfd, events);

    return 0;
}