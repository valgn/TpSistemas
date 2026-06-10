#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define MAX_BUFF 255


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

int main()
{
    char buff[MAX_BUFF];
    // creacion de sockets de escucha 
    int rc; // variable para chequear errores

    // Internet
    struct sockaddr_in sa;
    int listensock;
    int yes = 1;

    listensock = socket(AF_INET, SOCK_STREAM, 0); // socket por internet; AF_INET = ipv4

    if (listensock < 0) perror("socket");

    if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == 1) // opcion para que no se quede tomado el puerto una vez finaliza el programa
        perror("setsockopt");

    sa.sin_family = AF_INET;
    sa.sin_port = htons(5678); // puerto de prueba, puede ser cualquiera
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // ip local

    rc = bind(listensock, (struct sockaddr *)&sa, sizeof(sa));
    if (rc < 0) perror("bind");

    rc = listen(listensock, 100); // cola de hasta 100 conexiones, es arbitrario
    if (rc < 0) perror("listen");

    int clientfd = accept(listensock, NULL, NULL);

    fd_readline(clientfd, buff);

    printf("Message Recieved: %s\n", buff);

    return 0;
}