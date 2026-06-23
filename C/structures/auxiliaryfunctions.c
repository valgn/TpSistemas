#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxiliaryfunctions.h"
#include "data_types.h"

void *identity_copy(void *data)
{
    return data;
}

unsigned hash_job(void *source)
{
    Job *job = (Job *)source;
    unsigned h = (unsigned)job->clientfd;
    h = h * 31u + (unsigned)job->job_id;
    return h;
}

int comp_job(void *source1, void *source2)
{
    Job *job1 = (Job *)source1;
    Job *job2 = (Job *)source2;
 
    if (job1->clientfd < job2->clientfd) return -1;
    if (job1->clientfd > job2->clientfd) return  1;
    if (job1->job_id < job2->job_id) return -1;
    if (job1->job_id > job2->job_id) return  1;
    return 0;
}

unsigned hash_nodeRequest(void *source)
{
    NodeRequest *r = (NodeRequest *)source;
    unsigned h = (unsigned)r->client_fd;
    h = h * 31u + (unsigned)r->job_id;
    h = h * 11u + (unsigned)r->resource;
    return h;
}

int comp_nodeRequest(void *source1, void *source2)
{
    NodeRequest *r1 = (NodeRequest *)source1;
    NodeRequest *r2 = (NodeRequest *)source2;

    if (r1->client_fd == r2->client_fd && r1->job_id == r2->job_id && r1->resource == r2->resource)
        return 0;
    if (r1->client_fd != r2->client_fd) return r1->client_fd - r2->client_fd;
    if (r1->job_id != r2->job_id) return r1->job_id - r2->job_id;
    return r1->resource - r2->resource;
}

void destroy_job(void *source)
{
    Job *job = (Job *)source;
    if (!job) return;
    glist_destroy(job->requests, destroy_request);
    free(job);
}

void destroy_nodeRequest(void *source)
{
    free(source);
}

void destroy_request(void *source)
{
    free(source);
}

