#define _GNU_SOURCE

#include "agent.h"

char my_ip[INET_ADDRSTRLEN] = {0};

int ERLANG_TCP_PORT = 5678;
int NODE_TCP_PORT = 5679;
int UDP_PORT = 12529;

int available_cpu;
int available_mem;
int available_gpu;

int MAX_CPU = 0;
int MAX_MEM = 0;
int MAX_GPU = 0;

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t available_job_in_queue = PTHREAD_COND_INITIALIZER;
pthread_cond_t resources_freed_cond = PTHREAD_COND_INITIALIZER;

Queue* global_queue = NULL;
HashTable hashtable_jobs = NULL;
HashTable hashtable_nodeRequests = NULL;
GList active_nodes = NULL;
/*  -----------------------------------------------------------------------
    * Main loop of the server. It uses epoll to monitor multiple file descriptors for events, including incoming connections from Erlang clients, node clients, and UDP announcements. It also handles periodic tasks such as checking for down nodes and broadcasting the node's available resources.
    * Parameters: int epfd (the epoll file descriptor), struct epoll_event *events (array of epoll events), int udpsockfd (the UDP socket file descriptor), struct sockaddr_in udp_dest (the destination address for UDP broadcast).
    * Returns: void (Nothing).
 */
void server_running_main(int epfd, struct epoll_event *events,
                         int udpsockfd, struct sockaddr_in udp_dest)
{
    int rc;
    time_t last_announce = time(NULL);

    while (1)
    {
        // A timeout of 1000 milliseconds (1 second) is used to allow periodic tasks (like checking for down nodes) to be executed even if there are no events.
        int n_events = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (n_events < 0) { perror("epoll_wait"); continue; }

        for (int i = 0; i < n_events; i++)
        {
            EventData *event = (EventData *)events[i].data.ptr;
            if (!event) continue;

            switch (event->type)
            {
                case LISTENER_ERLANG:
                {
                    int clientfd = accept(event->fd, NULL, NULL);
                    if (clientfd < 0) { perror("accept erlang"); break; }
                    
                    set_nonblocking(clientfd);

                    EventData *client = malloc(sizeof(EventData));
                    client->fd   = clientfd;
                    client->type = CLIENT_ERLANG;

                    struct epoll_event ev;
                    ev.events   = EPOLLIN;
                    ev.data.ptr = client;

                    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
                    if (rc < 0) { perror("epoll_ctl erlang"); free(client); }
                    break;
                }

                case LISTENER_NODE:
                {
                    int clientfd = accept(event->fd, NULL, NULL);
                    if (clientfd < 0) { perror("accept node"); break; }

                    set_nonblocking(clientfd);

                    EventData *client = malloc(sizeof(EventData));
                    client->fd   = clientfd;
                    client->type = CLIENT_NODE;

                    struct epoll_event ev;
                    ev.events   = EPOLLIN;
                    ev.data.ptr = client;

                    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
                    if (rc < 0) { perror("epoll_ctl node"); free(client); }
                    break;
                }

                case CLIENT_ERLANG:
                {
                    if (handle_erlang_client(event->fd) == 0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, event->fd, NULL);
                        close(event->fd);
                        free(event); // It is secure to free it here since we handle the exit flow
                    }
                    break;
                }

                case CLIENT_NODE:
                {
                    if (handle_node_client(event->fd) == 0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, event->fd, NULL);
                        close(event->fd);
                        free(event);
                    }
                    break;
                }

                case UDP_NODE:
                {
                    handle_udp_announce(event->fd);
                    break;
                }

                case TIMER_EVENT:
                {
                    handle_request_timer(event->fd);
                    break;
                }
            }
        }

        // TIME TASKS (Outside the event loop to avoid delays)
        time_t now = time(NULL);
        
        // Verify timeouts of the nodes in the cluster
        remove_down_nodes(now);

        // Send the Keep-Alive/ANNOUNCE via UDP every 5 seconds
        if (now - last_announce >= ANNOUNCE_INTERVAL)
        {
            broadcast_udp_node(udpsockfd, udp_dest, available_cpu, available_mem, available_gpu, NODE_TCP_PORT);
            last_announce = now;
        }
    }
}

/*  -----------------------------------------------------------------------
    * Entry point of the program. It initializes the server, sets up sockets for Erlang clients, node clients, and UDP announcements, starts the scheduler thread, and enters the main event loop to handle incoming connections and requests.
    * Parameters: int argc (the number of command-line arguments), char *argv[] (the array of command-line argument strings).
    * Returns: int (0 on successful execution, non-zero on error).
 */
int main(int argc, char *argv[])
{
    
    if (argc == 7) {
        strcpy(my_ip, argv[1]);
        ERLANG_TCP_PORT = atoi(argv[2]);
        NODE_TCP_PORT = atoi(argv[3]);
        MAX_CPU = atoi(argv[4]);
        MAX_MEM = atoi(argv[5]);
        MAX_GPU = atoi(argv[6]);
        available_cpu = MAX_CPU;
        available_mem = MAX_MEM;
        available_gpu = MAX_GPU;
    } else if (argc > 1) {
        fprintf(stderr, "Uso: %s [MY_IP ERLANG_PORT NODE_PORT MAX_CPU MAX_MEM MAX_GPU]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    global_queue = create_queue();
    hashtable_jobs = hashTable_create(50, identity_copy, comp_job,
                                                destroy_job, hash_job);
    hashtable_nodeRequests = hashTable_create(50, identity_copy, comp_nodeRequest,
                                               destroy_nodeRequest, hash_nodeRequest);

    /* Sockets */
    int erlanglsockfd = setup_listening_erlang_sock(ERLANG_TCP_PORT);
    int nodeslsockfd  = setup_listening_nodes_sock(NODE_TCP_PORT);
    int udpsockfd     = setup_udp_node_sock(UDP_PORT);
    struct sockaddr_in udp_dest = create_udp_broadcast_dest(UDP_PORT);

    int timerfd = setup_request_timerfd();

    // We launch the scheduler in its own thread
    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler_loop, NULL);

    /* Epoll creation */
    int epfd = epoll_create1(0);
    if (epfd == -1) { perror("epoll_create1"); return 1; }

    // We register the Erlang listener, Node listener, UDP socket, and timerfd with epoll
    EventData *erlang_listener = malloc(sizeof(EventData));
    erlang_listener->fd   = erlanglsockfd;
    erlang_listener->type = LISTENER_ERLANG;

    struct epoll_event ev_erlang;
    ev_erlang.events   = EPOLLIN;
    ev_erlang.data.ptr = erlang_listener;
    epoll_ctl(epfd, EPOLL_CTL_ADD, erlanglsockfd, &ev_erlang);

    EventData *nodes_listener = malloc(sizeof(EventData));
    nodes_listener->fd   = nodeslsockfd;
    nodes_listener->type = LISTENER_NODE;

    struct epoll_event ev_nodes;
    ev_nodes.events   = EPOLLIN;
    ev_nodes.data.ptr = nodes_listener;
    epoll_ctl(epfd, EPOLL_CTL_ADD, nodeslsockfd, &ev_nodes);

    EventData *udp_event = malloc(sizeof(EventData));
    udp_event->fd   = udpsockfd;
    udp_event->type = UDP_NODE;

    struct epoll_event ev_udp;
    ev_udp.events   = EPOLLIN;
    ev_udp.data.ptr = udp_event;
    epoll_ctl(epfd, EPOLL_CTL_ADD, udpsockfd, &ev_udp);

    EventData *timer_event = malloc(sizeof(EventData));
    timer_event->fd   = timerfd;
    timer_event->type = TIMER_EVENT;

    struct epoll_event ev_timer;
    ev_timer.events   = EPOLLIN;
    ev_timer.data.ptr = timer_event;
    epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &ev_timer);

    struct epoll_event events[MAX_EVENTS];

    // Initial broadcast to announce this node's presence and wait for 2 seconds to discover already active nodes
    broadcast_udp_node(udpsockfd, udp_dest, available_cpu, available_mem, available_gpu, NODE_TCP_PORT);

    int n = epoll_wait(epfd, events, MAX_EVENTS, 2000);
    for (int i = 0; i < n; i++)
    {
        EventData *ev = (EventData *)events[i].data.ptr;
        if (ev->type == UDP_NODE)
            handle_udp_announce(ev->fd);
    }

    // Main loop of the server
    server_running_main(epfd, events, udpsockfd, udp_dest);

    return 0;
}