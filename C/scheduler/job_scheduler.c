#include "../wrapper.h"


void *scheduler_loop(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&resource_mutex);

    while (1)
    {
        while (empty_queue(global_queue))
            pthread_cond_wait(&available_job_in_queue, &resource_mutex);

        int q_size = 0;
        Nodo *temp = global_queue->first;
        while (temp) { q_size++; temp = temp->next; }

        int rotated_count = 0;
        int item_processed = 0;

        while (rotated_count < q_size && !item_processed)
        {
            UnifiedRequest* ureq = (UnifiedRequest*)global_queue->first->data;

            if (ureq->origin == REQ_ERLANG)
            {
                Job *job = ureq->data.erlang_job;

                if (job_has_impossible_local_request(job))
                {
                    dequeue(global_queue, identity_copy, NULL);
                    int fd_dest = job->clientfd;
                    
                    pthread_mutex_unlock(&resource_mutex);
                    if (fd_dest != -1) {
                        char response[64];
                        snprintf(response, sizeof(response), "JOB_DENIED %d\n", job->job_id);
                        write(fd_dest, response, strlen(response));
                    }
                    pthread_mutex_lock(&resource_mutex);

                    printf("[SCHEDULER] Job %d DENIED (Capacity exceeded)\n", job->job_id);
                    glist_destroy(job->requests, destroy_request);
                    free(job);
                    free(ureq); 
                    
                    item_processed = 1;
                    continue;
                }

                if (!all_local_resources_available(job))
                {
                    if (global_queue->first != global_queue->last)
                    {
                        Nodo *head = global_queue->first;
                        global_queue->first = head->next;
                        global_queue->last->next = head;
                        global_queue->last = head;
                        head->next = NULL;
                    }
                    rotated_count++;
                    continue;
                }

                commit_local_resources(job);
                job->is_processing = 1; 
                
                dequeue(global_queue, identity_copy, NULL);
                free(ureq);

                pthread_mutex_unlock(&resource_mutex);
                
                int remote_success = 1;
                GNode *curr = job->requests;

                while (curr)
                {
                    ErlangRequest *req = (ErlangRequest *)curr->data;
                    if (!is_local_node(req->ip))
                    {
                        int remote_port = 0;
                        GNode *n_iter = active_nodes;
                        while (n_iter != NULL) {
                            CNode *node = (CNode *)n_iter->data;
                            if (strcmp(node->ip, req->ip) == 0) {
                                remote_port = node->port;
                                break;
                            }
                            n_iter = n_iter->next;
                        }

                        if (remote_port == 0) {
                            printf("[SCHEDULER] Fail: Remote node %s not found.\n", req->ip);
                            remote_success = 0;
                            break;
                        }

                        int fd = connect_to_node(req->ip, remote_port);
                        if (fd < 0) {
                            remote_success = 0;
                            break;
                        }
                        req->remote_fd = fd;

                        struct timeval tv;
                        tv.tv_sec = 5;
                        tv.tv_usec = 0;
                        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

                        char msg[128];
                        snprintf(msg, sizeof(msg), "RESERVE %d %s %d\n",
                                 job->job_id,
                                 req->resource == CPU ? "cpu" : req->resource == MEM ? "mem" : "gpu",
                                 req->amount);
                        
                        if (write(fd, msg, strlen(msg)) <= 0) {
                            close(fd);
                            req->remote_fd = -1;
                            remote_success = 0;
                            break;
                        }

                        char response[64] = {0};
                        int bytes = read(fd, response, sizeof(response) - 1);
                        
                        if (bytes <= 0 || strstr(response, "GRANTED") == NULL) {
                            close(fd);
                            req->remote_fd = -1;
                            remote_success = 0;
                            break;
                        }
                    }
                    curr = curr->next;
                }

                pthread_mutex_lock(&resource_mutex);

                if (job->clientfd == -1)
                {
                    printf("[SCHEDULER] Aborting Job %d: Client disconnected\n", job->job_id);
                    rollback_remote_resources_until(job, remote_success ? NULL : curr);
                    rollback_local_resources(job);
                    
                    glist_destroy(job->requests, destroy_request);
                    free(job);
                    
                    pthread_cond_broadcast(&resources_freed_cond); 
                    item_processed = 1;
                    continue;
                }

                if (!remote_success)
                {
                    rollback_remote_resources_until(job, curr);
                    rollback_local_resources(job);
                    
                    int client_fd = job->clientfd;
                    char response[64];
                    snprintf(response, sizeof(response), "JOB_TIMEOUT %d\n", job->job_id);
                    
                    pthread_mutex_unlock(&resource_mutex);
                    write(client_fd, response, strlen(response));
                    pthread_mutex_lock(&resource_mutex);

                    printf("[SCHEDULER] Job %d ABORTED (Timeout or remote failure)\n", job->job_id);
                    glist_destroy(job->requests, destroy_request);
                    free(job);
                    
                    pthread_cond_broadcast(&resources_freed_cond);
                    item_processed = 1;
                    continue;
                }

                // ÉXITO
                hashTable_insert(hashtable_jobs, job);

                int client_fd = job->clientfd;
                char response[64];
                snprintf(response, sizeof(response), "JOB_GRANTED %d\n", job->job_id);
                
                pthread_mutex_unlock(&resource_mutex);
                write(client_fd, response, strlen(response));
                pthread_mutex_lock(&resource_mutex);

                printf("[SCHEDULER] Job %d GRANTED\n", job->job_id);
                item_processed = 1;
            }
            else if (ureq->origin == REQ_NODE)
            {
                NodeRequest *r = ureq->data.node_req;

                if (max_resource(r->resource) < r->amount)
                {
                    dequeue(global_queue, identity_copy, NULL);
                    char msg[32];
                    snprintf(msg, sizeof(msg), "DENIED %d\n", r->job_id);
                    write(r->client_fd, msg, strlen(msg));
                    free(r);
                    free(ureq);
                    
                    item_processed = 1;
                    continue;
                }

                if (available_resource(r->resource) < r->amount)
                {
                    if (global_queue->first != global_queue->last)
                    {
                        Nodo *head = global_queue->first;
                        global_queue->first = head->next;
                        global_queue->last->next = head;
                        global_queue->last = head;
                        head->next = NULL;
                    }
                    rotated_count++;
                    continue;
                }

                switch (r->resource)
                {
                    case CPU: available_cpu -= r->amount; break;
                    case MEM: available_mem -= r->amount; break;
                    case GPU: available_gpu -= r->amount; break;
                }

                register_remote_allocation(r->client_fd, r);

                char msg[32];
                snprintf(msg, sizeof(msg), "GRANTED %d\n", r->job_id);
                write(r->client_fd, msg, strlen(msg));

                dequeue(global_queue, identity_copy, NULL);
                free(r);
                free(ureq);
                
                item_processed = 1;
            }
        }

        if (!item_processed)
        {
            pthread_cond_wait(&resources_freed_cond, &resource_mutex);
        }
    }
    return NULL;
}


int all_local_resources_available(Job *job)
{
    GNode *curr = job->requests;
    while (curr)
    {
        ErlangRequest *req = (ErlangRequest *)curr->data;
        if (is_local_node(req->ip) && available_resource(req->resource) < req->amount)
            return 0;
        curr = curr->next;
    }
    return 1;
}


int job_has_impossible_local_request(Job *job)
{
    GNode *curr = job->requests;
    while (curr)
    {
        ErlangRequest *req = (ErlangRequest *)curr->data;
        if (is_local_node(req->ip) && req->amount > max_resource(req->resource))
            return 1;
        curr = curr->next;
    }
    return 0;
}


void handle_request_timer(int timerfd)
{
    uint64_t expirations;
    if (read(timerfd, &expirations, sizeof(expirations)) != sizeof(expirations))
        return;

    pthread_mutex_lock(&resource_mutex);
    int expired = expire_pending_node_requests();
    if (expired)
        pthread_cond_broadcast(&resources_freed_cond);
    pthread_mutex_unlock(&resource_mutex);
}


int setup_request_timerfd()
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd < 0)
    {
        perror("timerfd_create");
        return -1;
    }

    struct itimerspec its;
    its.it_interval.tv_sec = NODE_REQUEST_TIMEOUT_INTERVAL;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = NODE_REQUEST_TIMEOUT_INTERVAL;
    its.it_value.tv_nsec = 0;

    if (timerfd_settime(tfd, 0, &its, NULL) < 0)
    {
        perror("timerfd_settime");
        close(tfd);
        return -1;
    }

    return tfd;
}
