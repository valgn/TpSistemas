#define _GNU_SOURCE

#include "../wrapper.h"

/* ------------------------------------------------------------------------------------------
    * Reads one character at a time from the provided fd, then stores the result in a buffer.
    * Parameters: fd: file descriptor to read from, buf: buffer to store the result.
    * Returns: the number of characters read, or -1 if there was an error, or 0 if the connection was closed.
*/
int fd_readline(int fd, char *buf)
{
    int rc;
    int i = 0;

    while (1) 
    {
        rc = read(fd, buf + i, 1);
        
        if (rc > 0) 
        {
            if (buf[i] == '\n')
            {
                i++;
                break;
            }
            i++;
        } 
        else if (rc == 0) 
        {
            // EOF: Client closed the connection.
            if (i > 0) break;
            return 0;
        } 
        else 
        {
            // rc < 0 (Error)
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {

                // Socket is non-blocking and data is still in transit. 
                // Yield CPU for 1 millisecond to wait for the rest of the packet.
                usleep(1000); 
                continue;
            }
            return -1; // Error real de I/O
        }
    }

    return i;
}

/* ------------------------------------------------------------------------------------------
    * Copies the content of an ErlangRequest structure to a new one.
    * Parameters: data: pointer to the original ErlangRequest structure.
    * Returns: a pointer to the new ErlangRequest structure, or NULL if memory allocation fails.
*/
void *copy_request(void *data)
{
    ErlangRequest *req = (ErlangRequest *)data;
    ErlangRequest *copy = malloc(sizeof(ErlangRequest));
    if (!copy) return NULL;
    *copy = *req;
    return copy;
}

/* ------------------------------------------------------------------------------------------
*/
PetitionInfo* parse_erlang_petition(int clientfd)
{
    PetitionInfo *info = malloc(sizeof(PetitionInfo));

    char buff[MAX_BUFF];
    int read_characters = fd_readline(clientfd, buff);
    
    if (read_characters <= 0)
    {
        // Client disconnected or error occurred
        info->command = DISCONNECT;
        return info;
    }

    buff[strcspn(buff, "\n")] = '\0';

    char *saveptr1;
    char *saveptr2;

    char *token = strtok_r(buff, " ", &saveptr1);

    if(token == NULL)
    {
        info->command = INVALID;
        return info;
    }

    if(!strcmp(token, "JOB_REQUEST"))
    {
        info->command = JOB_REQUEST;
        token = strtok_r(NULL, " ", &saveptr1);
        if(token == NULL)
        {
            info->command = INVALID;
            return info;
        }
        int job_id = atoi(token);

        if(!job_id)
        {
            info->command = INVALID;
            return info;
        }

        GList request_list = glist_create();
        
        token = strtok_r(NULL, " ", &saveptr1);

        if(token == NULL)
        {
            info->command = INVALID;
            return info;
        }

        while(token != NULL)
        {
            ErlangRequest *request_structure = malloc(sizeof(ErlangRequest));

            char *request = strtok_r(token, ":", &saveptr2);
            if(request == NULL)
            {
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }

            if (request[0] == '@') request++;
            strncpy(request_structure->ip, request, INET_ADDRSTRLEN);
            request_structure->ip[INET_ADDRSTRLEN - 1] = '\0';

            request = strtok_r(NULL, ":", &saveptr2);
            if(request == NULL)
            {
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            if(!strcmp(request, "cpu")) request_structure->resource = CPU;
            else if(!strcmp(request, "mem")) request_structure->resource = MEM;
            else if(!strcmp(request, "gpu")) request_structure->resource = GPU;
            else
            {
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            

            request = strtok_r(NULL, ":", &saveptr2);
            if(request == NULL)
            {
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            int amount = atoi(request);
            if(!amount)
            {
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            request_structure->amount = amount;
            request_structure->remote_fd = -1;

            request_list = glist_addFront(request_list, request_structure, copy_request); 

            token = strtok_r(NULL, " ", &saveptr1);
        }

        Job *job_info = malloc(sizeof(Job));
        job_info->job_id = job_id;
        job_info->requests = request_list;
        job_info->clientfd = clientfd;
        job_info->is_processing = 0;

        info->structure = job_info;
    }

    else if(!strcmp(token, "JOB_RELEASE"))
    {
        info->command = JOB_RELEASE;
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            info->command = INVALID;
            return info;
        }

        int job_id = atoi(token); 
        if(!job_id)
        {

            info->command = INVALID;
            return info;
        }

        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {

            info->command = INVALID;
            return info;
        }

        int *job_id_info_ptr = malloc(sizeof(int));
        *job_id_info_ptr = job_id;

        info->structure = job_id_info_ptr;
    } 

    else if(!strcmp(token, "JOB_STATUS"))
    {
        info->command = JOB_STATUS;
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            // Error (indicate invalid structure) and delete list
            info->command = INVALID;
            return info;
        }

        int job_id = atoi(token);
        if(!job_id)
        {
            info->command = INVALID;
            return info;
        }

        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {
            info->command = INVALID;
            return info;
        }

        int *job_id_info_ptr = malloc(sizeof(int));
        *job_id_info_ptr = job_id;

        info->structure = job_id_info_ptr;
    }

    else if(!strcmp(token, "GET_NODES"))
    {
        info->command = GET_NODES;
        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {
            info->command = INVALID;
            return info;
        }
    }

    else
    {
        info->command = INVALID;
    }

    return info;
}

/* ------------------------------------------------------------------------------------------
    * Parses a petition from a node client and returns a PetitionInfo structure.
    * Parameters: clientfd: file descriptor of the client socket.
    * Returns: a pointer to a PetitionInfo structure containing the parsed information.
*/
PetitionInfo *parse_node_petition(int clientfd)
{
    
    PetitionInfo *info = malloc(sizeof(PetitionInfo));

    char buff[MAX_BUFF];
    int n = fd_readline(clientfd, buff);

    if (n <= 0)
    {
        info->command = DISCONNECT;
        return info;
    }

    buff[strcspn(buff, "\n")] = '\0';

    char *saveptr;
    char *token = strtok_r(buff, " ", &saveptr);

    if (!strcmp(token, "RESERVE"))
    {
        info->command = RESERVE;

        NodeRequest *req = malloc(sizeof(NodeRequest));
        req->client_fd = clientfd;

        token = strtok_r(NULL, " ", &saveptr);
        req->job_id = atoi(token);

        token = strtok_r(NULL, " ", &saveptr);
        if (!strcmp(token, "cpu")) req->resource = CPU;
        else if (!strcmp(token, "mem")) req->resource = MEM;
        else if (!strcmp(token, "gpu")) req->resource = GPU;

        token = strtok_r(NULL, " ", &saveptr);
        req->amount = atoi(token);

        token = strtok_r(NULL, " ", &saveptr);

        info->structure = req;
    }

    else if (!strcmp(token, "RELEASE"))
    {
        info->command = RELEASE;

        token = strtok_r(NULL, " ", &saveptr);
        int *job_id = malloc(sizeof(int));
        *job_id = atoi(token);

        info->structure = job_id;
    }

    return info;
}