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

#include "parse_handler.h"

#define MAX_CPU 4
#define MAX_MEM 8192
#define MAX_GPU 2

int available_cpu = MAX_CPU;
int available_mem = MAX_MEM;
int available_gpu = MAX_GPU;

void handle_client(int client_conn_fd)
{
    // llamar a parse_erlang_petition
}

int setup_listening_sock()
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

void server_running_main(int epfd, struct epoll_event *events, int lsockfd)
{
    int rc; // variable para chequear errores
    while(1)
    {
        int n_events = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n_events < 0) perror("epoll_wait");

        for (int i = 0; i < n_events; i++)
        {
            int fd = events[i].data.fd;
            if (fd == lsockfd) // si el evento vino del socket de escucha
            {
                int clientfd = accept(lsockfd, NULL, NULL);
                
                struct epoll_event ev_client;

                ev_client.events = EPOLLIN; // EPOLLIN = hay datos para leer
                ev_client.data.fd = clientfd;

                rc = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev_client); // agregamos a la conexion del cliente al epoll
                if (rc < 0) perror("epoll_ctl");
            }
            else if (fd != lsockfd) // si el evento vino de algo que no es el socket de escucha (un cliente)
            {
                handle_client(fd);
                close(fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // sacamos al cliente del epoll una vez terminamos de handlearlo
            }
        }
    }
}

int main()
{
    int rc; // variable para chequear errores

    int lsockfd = setup_listening_sock(); // preparamos el socket de escucha

    // empezamos epoll
    int epfd = epoll_create1(0); // creo objeto epoll
    if(epfd == -1) perror("epoll_create1");

    struct epoll_event ev; // creamos el objeto evento para el epoll que esta ligado al socket de escucha

    ev.events = EPOLLIN;
    ev.data.fd = lsockfd;

    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, lsockfd, &ev); // agregar el socket de escucha (internet) al epoll, y que avise cuando tenga conexiones pendientes
    if (rc < 0) perror("epoll_ctl");

    struct epoll_event events[MAX_EVENTS]; // estructura que nos va a almacenar la informacion de los eventos que se dispararon, hasta un maximo de MAX_EVENTS (arbitrario)

    server_running_main(epfd, events, lsockfd);

    return 0;
}