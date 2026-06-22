#ifndef ___DATA_TYPES_H___ 
#define ___DATA_TYPES_H___

#include <time.h>
#include <netinet/in.h>
#include "structures/glist.h"

typedef enum _Command
{
    JOB_REQUEST,
    JOB_RELEASE,
    JOB_STATUS,
    RESERVE,
    RELEASE,
    INVALID,
    GET_NODES,
    DISCONNECT
} Command;

typedef enum _Resource
{
    CPU,
    MEM,
    GPU
} Resource;

typedef enum {
    LISTENER_ERLANG,
    LISTENER_NODE,
    CLIENT_ERLANG,
    CLIENT_NODE,
    UDP_NODE
} EventType;

typedef struct {
    EventType type;
    int fd;
} EventData;

typedef enum {
    REQ_PENDING,
    REQ_GRANTED,
    REQ_DENIED,
    REQ_WAITING
} ReqState;

typedef struct _erlangRequest
{
    char ip[INET_ADDRSTRLEN];
    Resource resource; 
    int amount;
    ReqState state;
} ErlangRequest;

typedef struct _nodeRequest{
    int job_id;
    Resource resource;
    int amount;
    int client_fd;
} NodeRequest;

typedef struct _Job{
    int job_id;
    GList requests;   
    int clientfd;           
    int pending;      
} Job;

typedef struct _PetitionInfo
{
    Command command;
    void* structure;
} PetitionInfo;

typedef struct _RemoteAllocation
{
    int fd;                      // socket del nodo remoto
    int job_id;                  // job que originó la reserva
    Resource resource;
    int amount;
} RemoteAllocation;

typedef struct _cNode
{
    char ip[INET_ADDRSTRLEN];
    int port;

    int cpu;
    int mem;
    int gpu;

    time_t last_seen;
} CNode;


#endif