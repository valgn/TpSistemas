#ifndef __UDP_DISCOVERY_H__
#define __UDP_DISCOVERY_H__

#include "../wrapper.h"



/*  -----------------------------------------------------------------------
    * Removes nodes from the active_nodes list that have not sent an announcement within the ANNOUNCE_TIMEOUT period. It iterates through the list, checking the last_seen timestamp of each node, and removes any nodes that are considered down.
    * Parameters: time_t now (the current time used to check for timeouts).
    * Returns: void (Nothing).
 */
void remove_down_nodes(time_t now);

/*  -----------------------------------------------------------------------
    * Handles incoming UDP announcements from other nodes. It receives a message, parses the node's IP address, TCP port, and available resources, and updates the list of active nodes accordingly. If the node is new, it adds it to the list; if it already exists, it updates its information.
    * Parameters: int udpfd (the UDP socket file descriptor).
    * Returns: void (Nothing).
 */
void handle_udp_announce(int udpfd);

/*  -----------------------------------------------------------------------
    * Broadcasts a UDP message announcing the node's available resources to the network. It constructs a message containing the node's TCP port and available CPU, memory, and GPU resources, and sends it to the specified destination address.
    * Parameters: int sockfd (the UDP socket file descriptor), struct sockaddr_in dest (the destination address for the broadcast), int available_cpu (the number of available CPU cores), int available_mem (the amount of available memory in MB), int available_gpu (the number of available GPU units), int NODE_TCP_PORT (the TCP port on which the node is listening).
    * Returns: void (Nothing).
 */
void broadcast_udp_node(int sockfd, struct sockaddr_in dest, int available_cpu, int available_mem, int available_gpu, int NODE_TCP_PORT);

/*  -----------------------------------------------------------------------
    * Creates a sockaddr_in structure for broadcasting UDP messages to the network. It sets the destination address to the broadcast address and the specified UDP port.
    * Parameters: int UDP_PORT (the UDP port to which the broadcast messages will be sent).
    * Returns: struct sockaddr_in (the destination address for UDP broadcast).
 */
struct sockaddr_in create_udp_broadcast_dest(int UDP_PORT);

/*  -----------------------------------------------------------------------
    * Sets up a UDP socket for broadcasting and receiving node announcements. It configures the socket for broadcast and binds it to the specified UDP port.
    * Parameters: int UDP_PORT (the UDP port on which the socket will listen for incoming announcements).
    * Returns: int (file descriptor of the UDP socket) on success, -1 on failure.
 */
int setup_udp_node_sock(int UDP_PORT);

#endif