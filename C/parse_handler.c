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

	while ((rc = read(fd, buf + i, 1)) > 0) {
		if (buf[i] == '\n')
			break;
		i++;
	}

	if (rc < 0)
		return rc;

	buf[i] = 0;
	return i;
}

/*
parse_erlang_petition: parses the input of the erlang client and returns a pointer to a info structure
*/
void *parse_erlang_petition(int clientfd)
{
    char buff[MAX_BUFF];
    int read_characters = fd_readline(clientfd, buff);
    if (read_characters < 0) return 0;

    buff[strcspn(buff, "\n")] = '\0'; // quitamos el \n

    char *saveptr1;
    char *saveptr2;

    char *token = strtok_r(buff, " ", &saveptr1);

    if(strcmp(token, "JOB_REQUEST"))
    {
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            // error (indicar estructura invalido)
        }
        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0
        if(!job_id)
        {
            // error (indicar estructura invalido)
        }
        
        token = strtok(NULL, " "); // ip/res/amount
        while(token != NULL)
        {
            char *request = strtok_r(token, ":", &saveptr2); // ip
            if(request == NULL)
            {
                // error (indicar estructura invalido) y eliminar lista
            }


            request = strtok_r(NULL, ":", &saveptr2); // resource
            if(request == NULL)
            {
                // error (indicar estructura invalido) y eliminar lista
            }
            if(strcmp(request, "cpu"))
            {

            }
            else if(strcmp(request, "mem"))
            {

            }
            else if(strcmp(request, "gpu"))
            {

            }
            else
            {
                // error (indicar estructura invalido) y eliminar lista
            }
            

            request = strtok_r(NULL, ":", &saveptr2); // amount
            if(request == NULL)
            {
                // error (indicar estructura invalido) y eliminar lista
            }
            int amount = atoi(request);
            if(!amount)
            {
                // error (indicar estructura invalido) y eliminar lista
            }

            // crear estructura request y almacenarla en la lista

            token = strtok_r(NULL, " ", &saveptr1);
        }
        
        
    }

    else if(strcmp(token, "JOB_RELEASE"))
    {
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            // error (indicar estructura invalido)
        }
        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0
        if(!job_id)
        {
            // error (indicar estructura invalido)
        }
        token = strtok(NULL, " ");
        if(token != NULL)
        {
            // error (indicar estructura invalido)
        }
        // crear estructura release
    } 

    else if(strcmp(token, "JOB_STATUS"))
    {
        token = strtok_r(NULL, " ", &saveptr1); // job_id
        if(token == NULL)
        {
            // error (indicar estructura invalido)
        }
        int job_id = atoi(token); // da 0 si no es un entero o si el entero es 0
        if(!job_id)
        {
            // error (indicar estructura invalido)
        }
        token = strtok(NULL, " ");
        if(token != NULL)
        {
            // error (indicar estructura invalido)
        }
        // crear estructura status
    }

    else
    {
        // error (indicar estructura invalido)
    }
}
