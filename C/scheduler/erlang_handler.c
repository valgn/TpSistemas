#include "../wrapper.h"


int handle_erlang_client(int client_conn_fd)
{
    PetitionInfo *info = parse_erlang_petition(client_conn_fd);

    // DISCONNECT: Abrupt or planned disconnection of the Erlang client
    if (info->command == DISCONNECT)
    {
        printf("[ERLANG] fd=%d disconnected. Cleaning structures...\n", client_conn_fd);
        
        pthread_mutex_lock(&resource_mutex);

        // 1. Clean the waiting queue ( global_queue )
        Nodo *curr_node = global_queue ? global_queue->first : NULL;
        Nodo *prev_node = NULL;
        while (curr_node)
        {
            UnifiedRequest *ureq = (UnifiedRequest *)curr_node->data;
            Nodo *next_node = curr_node->next;

            if (ureq->origin == REQ_ERLANG)
            {
                Job *job = ureq->data.erlang_job;
                if (job->clientfd == client_conn_fd)
                {
                    if (job->is_processing)
                    {
                        job->clientfd = -1;
                        prev_node = curr_node;
                    }
                    else
                    {
                        if (prev_node) prev_node->next = next_node;
                        else global_queue->first = next_node;

                        if (curr_node == global_queue->last)
                            global_queue->last = prev_node;

                        glist_destroy(job->requests, destroy_request);
                        free(job);
                        free(ureq);
                        free(curr_node);
                    }
                }
                else
                {
                    prev_node = curr_node;
                }
            }
            else
            {
                prev_node = curr_node;
            }
            curr_node = next_node;
        }

        // 2. Clean the Hash Table of Approved Jobs (GRANTED)
        clean_hash_jobs_for_client(client_conn_fd);

        // Wake up the scheduler in case the head of the queue changed
        pthread_cond_signal(&available_job_in_queue);
        pthread_mutex_unlock(&resource_mutex);


        free(info);
        return 0; // Makes the main remove the fd from epoll and close the socket
    }

    // INVALID: The petition was invalid, so we keep the connection open but do not process it
    if (info->command == INVALID) 
    {   
        free(info);
        return 1; 
    }

    // GET_NODES: We send the list of active nodes to the Erlang client
    if (info->command == GET_NODES) 
    { 
        char buffer[MAX_BUFF];

        int offset = 0;

        offset += snprintf(buffer + offset, MAX_BUFF - offset, "NODES ");

        GNode *curr = active_nodes;

        while (curr != NULL)
        {
            CNode *node = curr->data;

            offset += snprintf(buffer + offset, MAX_BUFF - offset, "%s:%d:cpu:%d:mem:%d:gpu:%d", node->ip, node->port, node->cpu, node->mem, node->gpu);

            if (curr->next != NULL) offset += snprintf(buffer + offset, MAX_BUFF - offset, ";");

            curr = curr->next;
        }

        offset += snprintf(buffer + offset, MAX_BUFF - offset, "\n");

        write(client_conn_fd, buffer, offset);

        free(info);

        return 1;
    }

    // JOB_REQUEST: The Erlang client is requesting to submit a job with specific resource requirements.
    if (info->command == JOB_REQUEST)
    {

        Job *job = (Job *)info->structure;
        job->clientfd = client_conn_fd;
        job->is_processing = 0; // MODIFICADO: Inicializado explícitamente en cero

        UnifiedRequest *ureq = malloc(sizeof(UnifiedRequest));
        ureq->origin = REQ_ERLANG;
        ureq->timestamp = time(NULL);
        ureq->data.erlang_job = job;

        printf("[ERLANG] fd=%d JOB_REQUEST job_id=%d\n", client_conn_fd, job->job_id);

        pthread_mutex_lock(&resource_mutex);
        enqueue(global_queue, ureq, identity_copy);
        pthread_cond_signal(&available_job_in_queue);
        pthread_mutex_unlock(&resource_mutex);

        free(info);
        return 1;
    }

    // JOB_RELEASE: The Erlang client is requesting to release a previously granted job.
    if (info->command == JOB_RELEASE)
    {
        int job_id = *((int *)info->structure);
        free(info->structure);
        free(info);

        printf("[ERLANG] fd=%d JOB_RELEASE job_id=%d\n", client_conn_fd, job_id);

        Job key_job;
        key_job.clientfd = client_conn_fd; 
        key_job.job_id = job_id;

        pthread_mutex_lock(&resource_mutex);
        Job *job = (Job *)hashTable_search(hashtable_jobs, &key_job);

        if (job)
        {
            rollback_local_resources(job);
            pthread_mutex_unlock(&resource_mutex);

            // Liberamos conexiones remotas fuera del mutex para no trabar el servidor
            rollback_remote_resources_until(job, NULL);

            pthread_mutex_lock(&resource_mutex);
            hashTable_delete(hashtable_jobs, &key_job);
            pthread_cond_broadcast(&resources_freed_cond);
            pthread_cond_signal(&available_job_in_queue);
            pthread_mutex_unlock(&resource_mutex);
        }

        else
        {
            pthread_mutex_unlock(&resource_mutex);
        }

        // Release any remote allocations associated with this client_fd/job_id.
        release_remote_allocation(client_conn_fd, job_id);
        return 1;
    }

    // JOB_STATUS: The Erlang client is requesting the status of a previously submitted job.
    if (info->command == JOB_STATUS)
    {
        int job_id = *((int *)info->structure);
        free(info->structure);
        free(info);

        Job key;
        key.clientfd = client_conn_fd;
        key.job_id = job_id;

        char response[64];
        pthread_mutex_lock(&resource_mutex);
        Job *job = (Job *)hashTable_search(hashtable_jobs, &key);

        if (job)
        {
            snprintf(response, sizeof(response), "JOB_STATUS %d GRANTED\n", job_id);
        }
        else
        {
            int in_queue = 0;
            Nodo *n = global_queue ? global_queue->first : NULL;
            while (n)
            {
                UnifiedRequest *ureq = (UnifiedRequest *)n->data;
                if (ureq->origin == REQ_ERLANG)
                {
                    Job *q_job = ureq->data.erlang_job;
                    if (q_job->job_id == job_id && q_job->clientfd == client_conn_fd)
                    {
                        in_queue = 1;
                        break;
                    }
                }
                n = n->next;
            }
            snprintf(response, sizeof(response), in_queue ? "JOB_STATUS %d WAITING\n" : "JOB_STATUS %d UNKNOWN\n", job_id);
        }
        pthread_mutex_unlock(&resource_mutex);

        write(client_conn_fd, response, strlen(response));
        return 1;
    }

    free(info);
    return 1;
}


void clean_hash_jobs_for_client(int fd)
{
    // We pass the hashtable, the predicate function with the business logic, and the fd packed as a pointer
    hashTable_remove_if(hashtable_jobs, predicate_clean_client_jobs, &fd);
}



int predicate_clean_client_jobs(void *data1, void *data2)
{
    Job *job = (Job*)data1;
    int target_fd = *(int*)data2;

    if (job->clientfd == target_fd) 
    {
        printf("[CLEANUP] Disconnect: Returning resources of job %d\n", job->job_id);
        
        GNode *curr = job->requests;
        while (curr != NULL) 
        {
            ErlangRequest *req = (ErlangRequest*)curr->data;

            if (is_local_node(req->ip)) 
            {
                switch (req->resource) 
                {
                    case CPU: available_cpu += req->amount; break;
                    case MEM: available_mem += req->amount; break;
                    case GPU: available_gpu += req->amount; break;
                }
            }
            else if (req->remote_fd >= 0)
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

        pthread_cond_broadcast(&resources_freed_cond);
        pthread_cond_signal(&available_job_in_queue);
        return 1;
    }
    return 0; 
}
