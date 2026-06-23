#include "../wrapper.h"


int handle_node_client(int client_conn_fd)
{
    PetitionInfo *info = parse_node_petition(client_conn_fd);

    // DISCONNECT: The node client has disconnected, so we clean up any pending requests and remote allocations associated with this client.
    if (info->command == DISCONNECT)
    {
        printf("[NODE] fd=%d DISCONNECTED\n", client_conn_fd);
        remove_pending_requests_for_fd(client_conn_fd);
        release_all_remote_requests_from_fd(client_conn_fd);
        free(info);
        return 0;
    }

    // RESERVE: The node client is requesting to reserve resources for a specific job. We check if the requested amount exceeds the maximum capacity of this node, and if so, we deny the request. Otherwise, we enqueue the request for processing by the scheduler.
    if (info->command == RESERVE)
    {
        NodeRequest *r = (NodeRequest *)info->structure;

        if (r->amount > max_resource(r->resource))
        {
            char msg[32];
            snprintf(msg, sizeof(msg), "DENIED %d\n", r->job_id);
            write(client_conn_fd, msg, strlen(msg));
            free(r);
            free(info);
            return 1;
        }

        UnifiedRequest *ureq = malloc(sizeof(UnifiedRequest));
        ureq->origin = REQ_NODE;
        ureq->timestamp = time(NULL);
        ureq->data.node_req = r;

        pthread_mutex_lock(&resource_mutex);

        enqueue(global_queue, ureq, identity_copy);
        pthread_cond_signal(&available_job_in_queue);

          pthread_mutex_unlock(&resource_mutex);

        free(info);
        return 1;
    }

    // RELEASE: The node client is requesting to release previously reserved resources for a specific job. We remove any pending requests associated with this client and job, and we also release the remote allocation for the specified job ID.
    if (info->command == RELEASE)
    {
        int job_id = *((int *)info->structure);
        free(info->structure);
        free(info);

        remove_pending_requests_for_fd(client_conn_fd);
        release_remote_allocation(client_conn_fd, job_id);
        return 1;
    }

    free(info);
    return 1;
}


int predicate_remote_allocations_from_fd(void *data, void *ctx)
{
    NodeRequest *req = (NodeRequest *)data;
    int target_fd = *(int *)ctx;
    
    if (req->client_fd == target_fd)
    {
        switch (req->resource)
        {
            case CPU: available_cpu += req->amount; break;
            case MEM: available_mem += req->amount; break;
            case GPU: available_gpu += req->amount; break;
        }
        return 1;
    }
    return 0;
}

