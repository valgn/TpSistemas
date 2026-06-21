#ifndef ___DATA_TYPES_H___ 
#define ___DATA_TYPES_H___

#include <time.h>
#include <netinet/in.h>

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

typedef struct _Request
{
    int job_id;
    char ip[INET_ADDRSTRLEN];
    Resource resource; 
    int amount;
} Request;

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