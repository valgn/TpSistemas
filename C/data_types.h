#ifndef ___DATA_TYPES_H___ 
#define ___DATA_TYPES_H___

typedef enum _ErlangCommand
{
    JOB_REQUEST,
    JOB_RELEASE,
    JOB_STATUS
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
    Resource recurso; 
    int amount;
}Request;

typedef struct _release
{
    int job_id;
}Release;

typedef struct _status
{
    int job_id;
}Status;

typedef struct _info
{
    ErlangCommand comando;
    void* estructura;
}Informacion;













#endif