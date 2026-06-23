#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "../wrapper.h"
#include "../structures/data_types.h"
#include "../structures/glist.h"

/*  -----------------------------------------------------------------------
    * Releases all remote resource allocations associated with a specific client connection file descriptor and job ID.
    * Parameters: int client_conn_fd (the file descriptor of the client), int job_id (the ID of the job).
    * Returns: void (Nothing).
 */
void release_remote_allocation(int client_conn_fd, int job_id);

/*  -----------------------------------------------------------------------
    * Releases all remote resource allocations associated with a specific client connection file descriptor.
    * Parameters: int client_conn_fd (the file descriptor of the client).
    * Returns: void (Nothing).
 */
void release_all_remote_requests_from_fd(int client_conn_fd);

/*  -----------------------------------------------------------------------
    * Registers a remote resource allocation for a given client connection.
    * Parameters: int client_conn_fd (the file descriptor of the client), NodeRequest *r (the resource request).
    * Returns: void
 */
void register_remote_allocation(int client_conn_fd, NodeRequest *r);

/*  -----------------------------------------------------------------------
    * Rolls back remote resource reservations by sending release messages to the corresponding nodes.
    * Parameters: Job* job (pointer to the job being rolled back), GNode* limit (pointer to the node in the request list ).
    * Returns: void
 */
void rollback_remote_resources_until(Job *job, GNode *limit);

/*  -----------------------------------------------------------------------
    * After the mutex is locked, adds back the local resources to the available pool.
    * Parameters: Job* job (pointer to the job being rolled back).
    * Returns: void
 */
void rollback_local_resources(Job *job);

/* ------------------------------------------------------------------------
    * After the mutex is locked and all local resources are available, subtracts the local resources from the available pool.
    * Parameters: Job* job (pointer to the job being granted).
    * Returns: void
 */
void commit_local_resources(Job *job);

/*  -------------------------------------------------------------------------
    * Checks the global queue for any pending node requests that have exceeded the timeout and removes them.
    * Parameters: None
    * Returns: int (1 if any requests were removed, 0 otherwise).
 */
int expire_pending_node_requests(void);

/*  -------------------------------------------------------------------------
    * Removes all pending requests in the global queue that originated from a specific client connection file descriptor.
    * Parameters: int client_conn_fd (the file descriptor of the client connection).
    * Returns: void (Nothing).
 */
void remove_pending_requests_for_fd(int client_conn_fd);

/* --------------------------------------------------------------------------
    * Auxiliary function to check if a given IP address corresponds to this node or not.
    * Parameters: char* ip (IP address in string format).
    * Returns: 1 if the IP corresponds to this node, 0 otherwise.
*/  
int is_local_node(char *ip);

/*  --------------------------------------------------------------------------
    * Auxiliary function to abstract maximum resource access.
    * Parameters: Resource r (CPU, MEM, GPU).
    * Returns: An integer representing the maximum resource of this node.
*/
int max_resource(Resource r);

/*  --------------------------------------------------------------------------
    * Auxiliary function to abstract resource access.
    * Parameters: Resource r (CPU, MEM, GPU)
    * Returns: An integer representing the available resource at the moment.
*/
int available_resource(Resource r);

#endif