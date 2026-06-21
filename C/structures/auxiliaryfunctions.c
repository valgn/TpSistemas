#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxiliaryfunctions.h"
#include "data_types.h"

// It doesn't make a phsycal copy, only return the original pointer.
void* copy_request (void* source ){
    Request * request = source ;
    return request; 
}

// void* p_copy_request (void* source ){
//     Request * copy = malloc(sizeof(request));
//     Request * request = source ;
//     copy->job_id = request->job_id;
//     copy->ip = strdup(request->ip);
//     copy->resource = request->resource;
//     copy->amount = request->amount;
//     return copy; 
// }

void destroy_request(void* source){
    Request * request = source;
    free(request->ip);
    free(request);
}


// Request *request
// unsigned hash_request(void* source){
//         Request * request = (Request * )source ;
//     return request->job_id;
    
//     else{
//         perror("no es un request");
//     } 
// }

// unsigned comp_request(void* source1 , void* source2){
//     Request * request1 = (Petition *) source1;
//      Request * request2 = (Petition *) source2;
//         if (request1->job_id == request2->job_id)
//         return 0;
//         else if(request1->job_id > request2->job_id )
//         return 1;
//         else 
//         return -1;
//     }
    
//     else{
//         perror("no es un request");
//     }


void *identity_copy(void *data)
{
    return data;
}



