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

#define MAX_CPU 3
#define MAX_MEM 4096
#define MAX_GPU 0

char my_ip[INET_ADDRSTRLEN] = "192.168.0.X";

int available_cpu = MAX_CPU;
int available_mem = MAX_MEM;
int available_gpu = MAX_GPU;

pthread_cond_t available_job_in_queue = PTHREAD_COND_INITIALIZER;
pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;

Queue* queue_jobs = queue_create();
HashTabla remote_allocations_by_fd = hashTable_create(50, copy_request, comp_request, destroy_request, hash_request);
HashTabla hashtable_jobs = hashTable_create(50, copy_request, comp_request, destroy_request, hash_request);
GList active_nodes = NULL;

void release_remote_allocation(int fd, int job_id)
{
    GList list = hashTable_search(remote_allocations_by_fd, &fd);
    if (!list) return;

    GNode *curr = list;

    while (curr)
    {
        RemoteAllocation *a = curr->data;

        if (a->job_id == job_id)
        {
            pthread_mutex_lock(&resource_mutex);

            switch (a->resource)
            {
                case CPU: available_cpu += a->amount; break;
                case MEM: available_mem += a->amount; break;
                case GPU: available_gpu += a->amount; break;
            }

            pthread_mutex_unlock(&resource_mutex);

            printf("[RELEASE] fd=%d job=%d\n", fd, job_id);

            // borrar nodo de lista (simplificado)
        }

        curr = curr->next;
    }
}

void release_all_remote_requests_from_fd(int fd)
{
    GList list = hashTable_search(remote_allocations_by_fd, &fd);

    if (!list) return;

    GNode *curr = list;

    while (curr)
    {
        RemoteAllocation *a = curr->data;

        pthread_mutex_lock(&resource_mutex);

        switch (a->resource)
        {
            case CPU: available_cpu += a->amount; break;
            case MEM: available_mem += a->amount; break;
            case GPU: available_gpu += a->amount; break;
        }

        pthread_mutex_unlock(&resource_mutex);

        free(a);
        curr = curr->next;
    }

    glist_destroy(list, identity_copy);
    hashTable_delete(remote_allocations_by_fd, &fd);

    printf("[CLEANUP] Freed all remote allocations for fd %d\n", fd);
}

void register_remote_allocation(int fd, NodeRequest *r)
{
    RemoteAllocation *alloc = malloc(sizeof(RemoteAllocation));

    alloc->fd = fd;
    alloc->job_id = r->job_id;
    alloc->resource = r->resource;
    alloc->amount = r->amount;

    GList list = hashTable_search(remote_allocations_by_fd, &fd);

    list = glist_addFront(list, alloc, identity_copy);

    hashTable_insert(remote_allocations_by_fd, &fd, list);
}

int available_resource(Resource r)
{
    switch (r)
    {
        case CPU: return available_cpu;
        case MEM: return available_mem;
        case GPU: return available_gpu;
    }
    return 0;
}

int max_resource(Resource r)
{
    switch (r)
    {
        case CPU: return MAX_CPU;
        case MEM: return MAX_MEM;
        case GPU: return MAX_GPU;
    }
    return 0;
}


int is_local_node(char *ip)
{
    return strcmp(ip, my_ip) == 0;
}


void scheduler_loop()
{
    while (1)
    {
        while(empty_queue(queue_jobs)) pthread_cond_wait(&available_job_in_queue, &resource_mutex);

        Job *job_a_intentar_cumplir = queue_jobs->first->data;

        ErlangRequest *request_a_intentar_cumplir = job_a_intentar_cumplir->requests->data;

        while(request_a_intentar_cumplir != NULL)
        {
            if(is_local_node(request_a_intentar_cumplir->ip))
            {
                if(request_a_intentar_cumplir->amount > max_resource(request_a_intentar_cumplir->resource))
                {
                    // el job no se puede cumplir nunca, lo saco de la cola y aviso al cliente
                    dequeue(queue_jobs, identity_copy, destroy_request);
                    write(job_a_intentar_cumplir->clientfd, "JOB_DENIED\n", 11);
                    break;
                }

                else if (available_resource(request_a_intentar_cumplir->resource) < request_a_intentar_cumplir->amount)
                {
                    // no se puede cumplir el job 
                    break;
                }
                else 
                {
                    
                }
            }
            else 
            {
                
            }
    
            request_a_intentar_cumplir = request_a_intentar_cumplir->next;
        }

        if (!request_a_intentar_cumplir) 
        {
            // mandas las req
        }


    }
}

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
    dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    //dest.sin_addr.s_addr = inet_addr("192.168.0.23");

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

    //"info->structure" contiene el job en caso de request, un puntero a entero (jobid) en caso de status/release o nada en caso de DISCONNECT/INVALID/GET_NODES

    if(info->command == DISCONNECT)
    {
        printf("CLIENT WITH FD %d DISCONNECTED\n", client_conn_fd);
        // liberar recursos asociados a ese cliente (asociados a su fd), y sacar lo que este en la cola de jobs
        //  CLAVE: liberar TODO lo que ese nodo tenía reservado
        // release_all_requests_from_fd(client_conn_fd);

        return 0;
    }

    else if(info->command == INVALID)
    {
        printf("INVALID REQUEST FROM FD %d\n", client_conn_fd);
        write(client_conn_fd, "INVALID_REQUEST\n", 16);
        return 1;
    }

    else if (info->command == GET_NODES)
    {
        char buffer[MAX_BUFF];
        int offset = 0;

        offset += snprintf(buffer + offset, MAX_BUFF - offset, "NODES ");

        GNode *curr = active_nodes;

        while (curr != NULL)
        {
            CNode *node = curr->data;

            offset += snprintf(buffer + offset, MAX_BUFF - offset, "%s:%d:cpu:%d:mem:%d:gpu:%d", node->ip, node->port, node->cpu, node->mem, node->gpu);

            if (curr->next != NULL) offset += snprintf(buffer + offset, MAX_BUFF - offset, ";");

            curr = curr->next;
        }

        offset += snprintf(buffer + offset, MAX_BUFF - offset, "\n");

        write(client_conn_fd, buffer, offset);

        return 1;
    }

    else if(info->command == JOB_RELEASE)
{
    int *job_id_recibido = (int*)info->structure;

    Job dummy;
    dummy.job_id = *job_id_recibido;

    // 1. Buscamos el Job primero para poder examinar sus recursos antes de borrarlo
    Job *job_a_liberar = hashTable_search(hashtable_jobs, &dummy);

    if (job_a_liberar != NULL) 
    {
        // 2. Recorremos la lista de requests del job para devolver recursos o avisar a remotos
        GNode *curr = job_a_liberar->requests; // requests es un GList (GNode*)
        
        while (curr != NULL) 
        {
            ErlangRequest *req = (ErlangRequest*)curr->data;

            if (is_local_node(req->ip)) 
            {
                // Es LOCAL: Modificamos las variables globales protegiéndolas con el mutex
                pthread_mutex_lock(&resource_mutex);
                
                switch (req->resource) 
                {
                    case CPU: available_cpu += req->amount; break;
                    case MEM: available_mem += req->amount; break;
                    case GPU: available_gpu += req->amount; break;
                }
                
                // Importante: Avisamos al scheduler_loop de que hay nuevos recursos libres
                pthread_cond_signal(&available_job_in_queue);
                pthread_mutex_unlock(&resource_mutex);
            } 
            else 
            {
                // Es REMOTO: Acá debes armar el buffer y enviar el mensaje TCP al nodo correspondiente
                // Ejemplo conceptual:
                // enviar_mensaje_release_remoto(req->ip, req->job_id, req->resource, req->amount);
                printf("[REMOTE RELEASE] Enviar mensaje de liberación a la IP: %s\n", req->ip);
            }

            curr = curr->next;
        }

        // 3. Una vez devueltos todos los recursos, lo borramos físicamente de la tabla hash
        // Esto llamará internamente a la función de destrucción de tu Job
        hashTable_delete(hashtable_jobs, &dummy);
        printf("Job %d liberado y eliminado con éxito.\n", dummy.job_id);
    } 
    else 
    {
        printf("Advertencia: Se intentó liberar el Job %d pero no se encontró en activos.\n", dummy.job_id);
    }
    
    return 1;
}

    else if(info->command == JOB_STATUS)
    {
        // devuelve estado
    }

    else if(info->command == JOB_REQUEST)
    {
        // SE ENCOLA EL JOB
        enqueue(queue_jobs,info->structure,identity_copy);
        pthread_cond_signal(&available_job_in_queue);
        return 1;
    }
}

int handle_node_client(int client_conn_fd)
{
    PetitionInfo *info = parse_node_petition(client_conn_fd);

    if (info->command == DISCONNECT)
    {
        printf("NODE DISCONNECTED FD %d\n", client_conn_fd);

        release_all_remote_requests_from_fd(client_conn_fd);

        return 0;
    }

    if (info->command == RESERVE)
    {
        NodeRequest *r = (NodeRequest*)info->structure;

        if (r->amount > max_resource(r->resource))
        {
            write(client_conn_fd, "DENIED\n", 7);
            return 1;
        }

        pthread_mutex_lock(&resource_mutex);

        if (available_resource(r->resource) < r->amount)
        {
            pthread_mutex_unlock(&resource_mutex);

            // A ESPERAR, TODAVIA NO HAY ESPACIO

            // write(client_conn_fd, "WAITING\n", 8);
            return 1;
        }

        switch (r->resource)
        {
            case CPU: available_cpu -= r->amount; break;
            case MEM: available_mem -= r->amount; break;
            case GPU: available_gpu -= r->amount; break;
        }

        pthread_mutex_unlock(&resource_mutex);

        register_remote_allocation(client_conn_fd, r);

        write(client_conn_fd, "GRANTED\n", 8);
        return 1;
    }

    if (info->command == RELEASE)
    {
        int job_id = *((int*)info->structure);

        release_remote_allocation(client_conn_fd, job_id);

        // write(client_conn_fd, "RELEASED\n", 9);
        return 1;
    }

    return 1;
}

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

    active_nodes = glist_addFront(active_nodes, newNode, identity_copy);
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