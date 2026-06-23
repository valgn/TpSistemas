#ifndef AGENT_H
#define AGENT_H

#include <netinet/in.h>
#include "wrapper.h"

extern char my_ip[INET_ADDRSTRLEN];

extern int ERLANG_TCP_PORT;
extern int NODE_TCP_PORT;
extern int UDP_PORT;

#endif