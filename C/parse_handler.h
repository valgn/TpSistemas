#ifndef __PARSE_HANDLER_H__
#define __PARSE_HANDLER_H__

#define MAX_BUFF 255

#include "structures/data_types.h"
#include "structures/auxiliaryfunctions.h"
// #include "structures/hashtable.h"
// #include "structures/queue.h"

PetitionInfo *parse_erlang_petition(int clientfd);

PetitionInfo *parse_node_petition(int clientfd);

#endif
