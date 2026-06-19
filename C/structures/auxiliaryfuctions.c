#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data_types.h"

// dont make a copy only return the original pointer
void* copy_request (void* source ){
    Request * request = source ;
    return request; 
}

void destroy_request(void* source){
    Request * request = source;
    free(request->ip);
    free(request);
}

