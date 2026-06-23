#ifndef ___DATA_TYPES_H___ 
#define ___DATA_TYPES_H___

#include <time.h>
#include <netinet/in.h>
#include "glist.h"

#define MAX_BUFF 255

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
    UDP_NODE,
    TIMER_EVENT
} EventType;

typedef struct {
    EventType type;
    int fd;
} EventData;

typedef struct _erlangRequest
{
    char ip[INET_ADDRSTRLEN];
    Resource resource; 
    int amount;
    int remote_fd;
} ErlangRequest;

typedef struct _nodeRequest{
    int job_id;
    Resource resource;
    int amount;
    int client_fd;
    time_t timestamp;
} NodeRequest;

typedef struct _Job{
    int job_id;
    GList requests;   
    int clientfd;
    int is_processing;           
} Job;

typedef enum {
    REQ_ERLANG,
    REQ_NODE
} RequestOrigin;

typedef struct _UnifiedRequest {
    RequestOrigin origin;
    time_t timestamp; 
    union {
        Job *erlang_job;
        NodeRequest *node_req;
    } data;
} UnifiedRequest;

typedef struct _PetitionInfo
{
    Command command;
    void* structure;
} PetitionInfo;

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