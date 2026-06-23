#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "../wrapper.h"

/*  -----------------------------------------------------------------------
    * Sets up a listening TCP socket for node clients. It creates a socket, enables address reuse, binds it to the specified node TCP port on all interfaces, and starts listening for incoming connections.
    * Parameters: int node_tcp_port (TCP port number for node clients).
    * Returns: int (file descriptor of the listening socket) on success, -1 on failure.
 */
int setup_listening_nodes_sock(int node_tcp_port);

/*  -----------------------------------------------------------------------
    * Sets up a listening TCP socket for Erlang clients. It creates a socket, sets it to non-blocking mode, enables address reuse, binds it to the specified Erlang TCP port on localhost, and starts listening for incoming connections.
    * Parameters: int erlang_tcp_port (TCP port number for Erlang clients).
    * Returns: int (file descriptor of the listening socket) on success, -1 on failure.
 */
int setup_listening_erlang_sock(int erlang_tcp_port);

/*  -------------------------------------------------------------------------
    * Establishes a TCP connection to a remote node given its IP and port.
    * Parameters: const char* ip (IP address in string format), int port (TCP port number).
    * Returns: The file descriptor or -1 on failure. 
 */
int connect_to_node(const char *ip, int port);

/* --------------------------------------------------------------------------
    * Sets a file descriptor to non-blocking mode.
    * Parameters: int fd (file descriptor to set).
    * Returns: 0 on success, -1 on failure.
*/
int set_nonblocking(int fd);

#endif