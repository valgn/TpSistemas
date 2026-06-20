#ifndef ___DATA_TYPES_H___ 
#define ___DATA_TYPES_H___

typedef enum _Command
{
    JOB_REQUEST,
    JOB_RELEASE,
    JOB_STATUS,
    RESERVE,
    RELEASE,
    INVALID,
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
    CLIENT_NODE
} EventType;

typedef struct {
    EventType type;
    int fd;
} EventData;

typedef struct _Request
{
    int job_id;
    char *ip;
    Resource resource; 
    int amount;
} Request;

typedef struct _PetitionInfo
{
    Command command;
    void* structure;
} PetitionInfo;



#endif