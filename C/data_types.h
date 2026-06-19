#ifndef ___DATA_TYPES_H___ 
#define ___DATA_TYPES_H___

typedef enum _ErlangCommand
{
    JOB_REQUEST,
    JOB_RELEASE,
    JOB_STATUS,
    INVALID
} ErlangCommand;

typedef enum _Resource
{
    CPU,
    MEM,
    GPU
} Resource;

typedef struct _request
{
    int job_id;
    char *ip;
    Resource resource; 
    int amount;
}Request;

typedef struct _info
{
    ErlangCommand command;
    void* structure;
}Informacion;



#endif