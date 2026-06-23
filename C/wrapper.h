#ifndef WRAPPER_H
#define WRAPPER_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "structures/queue.h"
#include "structures/hashtable.h"
#include "structures/data_types.h"
#include "structures/glist.h"
#include "structures/auxiliaryfunctions.h"
#include "network/tcp_server.h"
#include "network/parse_handler.h"
#include "network/udp_discovery.h"
#include "scheduler/node_handler.h"
#include "scheduler/job_scheduler.h"
#include "scheduler/resource_manager.h"
#include "scheduler/erlang_handler.h"

#include <pthread.h>
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
#include <sys/timerfd.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>


#define MAX_CONNECTIONS 100
#define MAX_EVENTS 1024
#define ANNOUNCE_INTERVAL 2
#define ANNOUNCE_TIMEOUT 15
#define NODE_REQUEST_TIMEOUT 60
#define NODE_REQUEST_TIMEOUT_INTERVAL 5


extern int MAX_CPU;
extern int MAX_MEM;
extern int MAX_GPU;

extern int available_cpu;
extern int available_mem;
extern int available_gpu;

extern char my_ip[INET_ADDRSTRLEN];

extern pthread_mutex_t resource_mutex;
extern pthread_cond_t available_job_in_queue;
extern pthread_cond_t resources_freed_cond;

extern Queue* global_queue;
extern HashTable hashtable_jobs;
extern HashTable hashtable_nodeRequests;
extern GList active_nodes;

#endif