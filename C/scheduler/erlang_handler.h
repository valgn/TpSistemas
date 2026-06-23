#ifndef ERLANG_HANDLER_H
#define ERLANG_HANDLER_H

#include "../wrapper.h"

/*  -----------------------------------------------------------------------
    * Handles incoming requests from an Erlang client. It processes different commands such as JOB_REQUEST, JOB_RELEASE, JOB_STATUS, and DISCONNECT, managing the job queue and resource allocation accordingly.
    * Parameters: int client_conn_fd (the file descriptor of the Erlang client connection).
    * Returns: int (0 if the client disconnected, 1 if the connection should remain open).
 */
int handle_erlang_client(int client_conn_fd);

/*  -------------------------------------------------------------------------
    * Removes all jobs from the hashtable that originated from a specific client connection file descriptor.
    * Parameters: int fd (the file descriptor of the client connection).
    * Returns: void (Nothing).
 */
void clean_hash_jobs_for_client(int fd);

/*  -------------------------------------------------------------------------
    * Predicate function to check if a job in the hashtable originated from a specific client connection file descriptor.
    * Parameters: void *data1 (pointer to the job), void *data2 (pointer to the target file descriptor).
    * Returns: int (1 if the job originated from the target fd, 0 otherwise).
 */
int predicate_clean_client_jobs(void *data1, void *data2);

#endif