#include "../wrapper.h"


void release_remote_allocation(int client_conn_fd, int job_id)
{
    pthread_mutex_lock(&resource_mutex);

    int resources_freed = 0;
    Resource types[] = {CPU, MEM, GPU};

    // Deterministic search over the 3 possible resources by initializing the key correctly
    for (int i = 0; i < 3; i++)
    {
        NodeRequest key;
        key.client_fd = client_conn_fd;
        key.job_id    = job_id;
        key.resource  = types[i]; 

        NodeRequest *found = (NodeRequest *)hashTable_search(hashtable_nodeRequests, &key);
        if (found)
        {
            switch (found->resource)
            {
                case CPU: available_cpu += found->amount; break;
                case MEM: available_mem += found->amount; break;
                case GPU: available_gpu += found->amount; break;
            }
            resources_freed = 1;
            
            hashTable_delete(hashtable_nodeRequests, &key);
        }
    }

    if (resources_freed)
    {
        printf("[REMOTE REL] Freed resources of fd=%d for Job %d. Waking scheduler up.\n", client_conn_fd, job_id);
        pthread_cond_broadcast(&resources_freed_cond);
    }

    pthread_mutex_unlock(&resource_mutex);
}


void release_all_remote_requests_from_fd(int client_conn_fd)
{
    pthread_mutex_lock(&resource_mutex);
    
    int prev_cpu = available_cpu, prev_mem = available_mem, prev_gpu = available_gpu;

    hashTable_remove_if(hashtable_nodeRequests,
                        predicate_remote_allocations_from_fd,
                        &client_conn_fd);
    
    // If any resources were freed by the predicate, notify the scheduler
    if (available_cpu > prev_cpu || available_mem > prev_mem || available_gpu > prev_gpu) 
    {
        pthread_cond_broadcast(&resources_freed_cond);
    }
    
    pthread_mutex_unlock(&resource_mutex);
}


void register_remote_allocation(int client_conn_fd, NodeRequest *r)
{
    NodeRequest *allocation = malloc(sizeof(NodeRequest));
    if (!allocation) return;

    allocation->client_fd = client_conn_fd;
    allocation->job_id    = r->job_id;
    allocation->resource  = r->resource;
    allocation->amount    = r->amount;

    // It is assumed that remote_allocations_by_fd uses a combined key
    // or handles collisions if a fd/job_id requests multiple resources.
    hashTable_insert(hashtable_nodeRequests, allocation);
    
    printf("[REMOTE REG] fd=%d reserved %d units of %s for Job %d\n", 
           client_conn_fd, r->amount,
           r->resource == CPU ? "cpu" : r->resource == MEM ? "mem" : "gpu", 
           r->job_id);
}


void rollback_remote_resources_until(Job *job, GNode *limit)
{
    GNode *curr = job->requests;
    while (curr != limit)
    {
        ErlangRequest *req = (ErlangRequest *)curr->data;
        if (!is_local_node(req->ip) && req->remote_fd >= 0)
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "RELEASE %d %s %d\n",
                     job->job_id,
                     req->resource == CPU ? "cpu" : req->resource == MEM ? "mem" : "gpu",
                     req->amount);
            write(req->remote_fd, msg, strlen(msg));
            
            close(req->remote_fd);
            req->remote_fd = -1;
        }
        curr = curr->next;
    }
}


void rollback_local_resources(Job *job)
{
    GNode *curr = job->requests;
    while (curr)
    {
        ErlangRequest *req = (ErlangRequest *)curr->data;
        if (is_local_node(req->ip))
        {
            switch (req->resource)
            {
                case CPU: available_cpu += req->amount; break;
                case MEM: available_mem += req->amount; break;
                case GPU: available_gpu += req->amount; break;
            }
        }
        curr = curr->next;
    }
}


void commit_local_resources(Job *job)
{
    GNode *curr = job->requests;
    while (curr)
    {
        ErlangRequest *req = (ErlangRequest *)curr->data;
        if (is_local_node(req->ip))
        {
            switch (req->resource)
            {
                case CPU: available_cpu -= req->amount; break;
                case MEM: available_mem -= req->amount; break;
                case GPU: available_gpu -= req->amount; break;
            }
        }
        curr = curr->next;
    }
}


int expire_pending_node_requests(void)
{
    if (global_queue == NULL) return 0;

    time_t now = time(NULL);
    Nodo *curr = global_queue->first;
    Nodo *prev = NULL;
    int removed = 0;

    while (curr)
    {
        UnifiedRequest *ureq = (UnifiedRequest *)curr->data;
        Nodo *next = curr->next;

        // Generic Timeout is evaluated using the timestamp of the wrapper structure
        if ((now - ureq->timestamp) >= NODE_REQUEST_TIMEOUT)
        {
            if (ureq->origin == REQ_NODE)
            {
                NodeRequest *req = ureq->data.node_req;
                char msg[32];
                snprintf(msg, sizeof(msg), "DENIED %d\n", req->job_id);
                write(req->client_fd, msg, strlen(msg));
                
                free(req);
            }
            else if (ureq->origin == REQ_ERLANG)
            {
                Job *job = ureq->data.erlang_job;
                
                // If the job is already being processed by the scheduler (waiting for remote resources),
                // should not be removed from this function. The scheduler handles its own network timeout.
                if (job->is_processing)
                {
                    prev = curr;
                    curr = next;
                    continue;
                }

                if (job->clientfd != -1)
                {
                    char response[64];
                    snprintf(response, sizeof(response), "JOB_TIMEOUT %d\n", job->job_id);
                    write(job->clientfd, response, strlen(response));
                }
                
                glist_destroy(job->requests, destroy_request);
                free(job);
            }

            // Common logic for dequeueing and freeing memory
            if (prev == NULL)
                global_queue->first = next;
            else
                prev->next = next;

            if (curr == global_queue->last)
                global_queue->last = prev;

            free(ureq);
            free(curr);
            removed = 1;
            curr = next;
            continue;
        }

        prev = curr;
        curr = next;
    }

    return removed;
}


void remove_pending_requests_for_fd(int client_conn_fd)
{
    if (!global_queue) return;

    Nodo *curr = global_queue->first;
    Nodo *prev = NULL;

    while (curr)
    {
        UnifiedRequest *ureq = (UnifiedRequest *)curr->data;
        Nodo *next = curr->next;

        if (ureq->origin == REQ_NODE &&
            ureq->data.node_req->client_fd == client_conn_fd)
        {
            if (prev == NULL)
                global_queue->first = next;
            else
                prev->next = next;

            if (curr == global_queue->last)
                global_queue->last = prev;

            free(ureq->data.node_req);
            free(ureq);
            free(curr);
        }
        else
        {
            prev = curr;
        }

        curr = next;
    }
}


int is_local_node(char *ip)
{
    return strcmp(ip, my_ip) == 0;
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