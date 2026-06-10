#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



int main()
{
    char buff[255] = "JOB_REQUEST 1001\n";

    char *saveptr1;
    char *saveptr2;

    buff[strcspn(buff, "\n")] = '\0'; // quitamos el \n

    char *token = strtok_r(buff, " ", &saveptr1); // JOB_REQUEST

    printf("Token: %s\n", token); 

    token = strtok_r(NULL, " ", &saveptr1); // 1001

    printf("Token: %s\n", token); 

    token = strtok_r(NULL, " ", &saveptr1); // req

    while(token != NULL)
    {
        printf("Token: %s\n", token);
        char *request = strtok_r(token, ":", &saveptr2);
        while (request != NULL)
        {
            printf("request: %s\n", request);
            request = strtok_r(NULL, ":", &saveptr2);
        }
        token = strtok_r(NULL, " ", &saveptr1);
    }


    return 0;
}