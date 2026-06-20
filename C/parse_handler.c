#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parse_handler.h"

/*
fd_readline: reads one character at a time from the provided fd, then stores the result in a buffer
*/
int fd_readline(int fd, char *buf)
{
	int rc;
	int i = 0;

	while ((rc = read(fd, buf + i, 1)) > 0) 
    {
		if (buf[i] == '\n')
        {
            i++;
			break;
        }
        
		i++;
	}

	if (rc <= 0)
		return rc;

	return i;
}

/*
parse_erlang_petition: parses the input of the erlang client and returns a pointer to a PetitionInfo structure
*/
PetitionInfo *parse_erlang_petition(int clientfd)
{
    PetitionInfo *info = malloc(sizeof(PetitionInfo));

    char buff[MAX_BUFF];
    int read_characters = fd_readline(clientfd, buff);
    
    if (read_characters <= 0)
    {
        // cliente cerró conexión o hubo desconexion abrupta
        info->command = DISCONNECT;
        return info;
    }

    buff[strcspn(buff, "\n")] = '\0'; // quitamos el \n

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
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            info->command = INVALID;
            return info;
        }
        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0

        if(!job_id)
        {
            info->command = INVALID;
            return info;
        }

        GList request_list = glist_create();
        
        token = strtok_r(NULL, " ", &saveptr1); // ip/res/amount

        if(token == NULL)
        {
            info->command = INVALID;
            return info;
        }

        while(token != NULL)
        {
            Request *request_structure = malloc(sizeof(Request));
            request_structure->job_id = job_id;

            char *request = strtok_r(token, ":", &saveptr2); // ip
            if(request == NULL)
            {
                // error (indicar estructura invalido) y eliminar lista
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            request_structure->ip = strdup(request);

            request = strtok_r(NULL, ":", &saveptr2); // resource
            if(request == NULL)
            {
                // error (indicar estructura invalido) y eliminar lista
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
                // error (indicar estructura invalido) y eliminar lista
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            

            request = strtok_r(NULL, ":", &saveptr2); // amount
            if(request == NULL)
            {
                // error (indicar estructura invalido) y eliminar lista
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            int amount = atoi(request);
            if(!amount)
            {
                // error (indicar estructura invalido) y eliminar lista
                info->command = INVALID;
                free(request_structure);
                glist_destroy(request_list, destroy_request);
                return info;
            }
            request_structure->amount = amount;

            request_list = glist_addFront(request_list, request_structure, copy_request); 

            token = strtok_r(NULL, " ", &saveptr1);
        }

        info->structure = request_list;
    }

    else if(!strcmp(token, "JOB_RELEASE"))
    {
        info->command = JOB_RELEASE;
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0
        if(!job_id)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {
            // error (indicar estructura invalido)
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
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0
        if(!job_id)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        int *job_id_info_ptr = malloc(sizeof(int));
        *job_id_info_ptr = job_id;

        info->structure = job_id_info_ptr;
    }

    else
    {
        info->command = INVALID;
    }

    return info;
}

/*
parse_node_petition: parses the input of the node client and returns a pointer to a PetitionInfo structure
*/
PetitionInfo *parse_node_petition(int clientfd)
{
    PetitionInfo *info = malloc(sizeof(PetitionInfo));

    char buff[MAX_BUFF];
    int read_characters = fd_readline(clientfd, buff);
    
    if (read_characters <= 0)
    {
        // cliente cerró conexión o hubo desconexion abrupta
        info->command = DISCONNECT;
        return info;
    }

    buff[strcspn(buff, "\n")] = '\0'; // quitamos el \n

    char *saveptr1;
    char *saveptr2;

    char *token = strtok_r(buff, " ", &saveptr1);

    if(token == NULL)
    {
        info->command = INVALID;
        return info;
    }

    if(!strcmp(token, "RESERVE"))
    {
        info->command = RESERVE;
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            info->command = INVALID;
            return info;
        }
        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0

        if(!job_id)
        {
            info->command = INVALID;
            return info;
        }

        Request *request_structure = malloc(sizeof(Request));
        request_structure->job_id = job_id;
        
        token = strtok_r(NULL, " ", &saveptr1); // resource

        if(token == NULL)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            free(request_structure);
            return info;
        }
        if(!strcmp(token, "cpu")) request_structure->resource = CPU;
        else if(!strcmp(token, "mem")) request_structure->resource = MEM;
        else if(!strcmp(token, "gpu")) request_structure->resource = GPU;

        token = strtok_r(NULL, " ", &saveptr1); // amount
        if(token == NULL)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            free(request_structure);
            return info;
        }
        int amount = atoi(token);
        if(!amount)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            free(request_structure);
            return info;
        }
        request_structure->amount = amount;

        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {
            info->command = INVALID;
            free(request_structure);
            return info;
        }

        info->structure = request_structure;
    }

    else if(!strcmp(token, "RELEASE"))
    {
        info->command = RELEASE;
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0
        if(!job_id)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        token = strtok_r(NULL, " ", &saveptr1);
        if(token != NULL)
        {
            // error (indicar estructura invalido)
            info->command = INVALID;
            return info;
        }

        int *job_id_info_ptr = malloc(sizeof(int));
        *job_id_info_ptr = job_id;

        info->structure = job_id_info_ptr;
    } 

    else
    {
        info->command = INVALID;
    }

    return info;
}