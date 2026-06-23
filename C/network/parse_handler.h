#ifndef __PARSE_HANDLER_H__
#define __PARSE_HANDLER_H__

#include "../wrapper.h"

/*  -----------------------------------------------------------------------------------------
    * Parses the input of the erlang client petition.
    * Parameters: clientfd: file descriptor of the client socket.
    * Returns: a pointer to a PetitionInfo structure containing the command and the relevant information for that command.
*/
PetitionInfo *parse_erlang_petition(int clientfd);

/*  -----------------------------------------------------------------------------------------
    * Parses the input of the node client.
    * Parameters: clientfd: file descriptor of the client socket.
    * Returns: a pointer to a PetitionInfo structure containing the command and the relevant information for that command.
*/
PetitionInfo *parse_node_petition(int clientfd);

#endif