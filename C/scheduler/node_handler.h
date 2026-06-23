#ifndef NODE_HANDLER_H
#define NODE_HANDLER_H

#include "../wrapper.h"

/*  -----------------------------------------------------------------------
    * Handles incoming requests from a node client. It processes different commands such as RESERVE, RELEASE, and DISCONNECT, managing the resource allocation and pending requests accordingly.
    * Parameters: int client_conn_fd (the file descriptor of the node client connection).
    * Returns: int (0 if the client disconnected, 1 if the connection should remain open).
 */
int handle_node_client(int client_conn_fd);

/*  -----------------------------------------------------------------------
    * Predicate function to check if a remote allocation in the hashtable originated from a specific client connection file descriptor.
    * Parameters: void *data (pointer to the NodeRequest), void *ctx (pointer to the target file descriptor).
    * Returns: int (1 if the allocation originated from the target fd, 0 otherwise).
 */
int predicate_remote_allocations_from_fd(void *data, void *ctx);


#endif