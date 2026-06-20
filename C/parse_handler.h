#ifndef __PARSE_HANDLER_H__
#define __PARSE_HANDLER_H__

#define MAX_BUFF 255

#include "structures/data_types.h"
#include "structures/glist.h"
#include "structures/auxiliaryfunctions.h"

PetitionInfo *parse_erlang_petition(int clientfd);

#endif
