#ifndef JOB_SCHEDULER_H
#define JOB_SCHEDULER_H

#include "../wrapper.h"

/*  -----------------------------------------------------------------------
    * Main loop of the scheduler thread. It continuously checks the global queue for pending jobs and node requests, processes them according to resource availability, and communicates with clients and remote nodes.
    * Parameters: void *arg (unused, can be NULL).
    * Returns: void* (always returns NULL).
 */
void *scheduler_loop(void *arg);

/* ------------------------------------------------------------------------
    * Checks if all local resources required by the job are currently available.
    * Parameters: Job* job (pointer to the job being evaluated).
    * Returns: 1 if all local resources are available, 0 if at least one is insufficient.
 */
int all_local_resources_available(Job *job);

/*  -------------------------------------------------------------------------
    * Verifies if any local request exceeds the maximum capacity of this node in order to deny the job immediately.
    * Parameters: Job* job (pointer to the job being evaluated).
    * Returns: 1 if there is at least one local request that exceeds the maximum, 0 otherwise.
 */
int job_has_impossible_local_request(Job *job);

/*  * -------------------------------------------------------------------------
    * Handles the expiration of pending requests by reading the timerfd and invoking the expiration logic.
    * Parameters: int timerfd (file descriptor of the timer).
    * Returns: void (Nothing).
 */
void handle_request_timer(int timerfd);

/*  -------------------------------------------------------------------------
    * Sets up a timerfd to periodically check for expired pending requests.
    * Parameters: None
    * Returns: int (file descriptor of the timerfd) on success, -1 on failure.
 */
int setup_request_timerfd();

#endif