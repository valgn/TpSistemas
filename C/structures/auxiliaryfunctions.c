#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxiliaryfunctions.h"
#include "data_types.h"

// It doesn't make a phsycal copy, only return the original pointer.
void* copy_request (void* source ){
    ErlangRequest * request = source ;
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
    ErlangRequest * request = source;
    free(request);
}


unsigned hash_jobs(void* source){
    
    Job * job = (job * )source ;
    return job->job_id;  
     
 }

int comp_request(void* source1 , void* source2){
    Job * job1 = (Job *) source1;
      Job * job2 = (Job *) source2;
         if (job1->job_id == job2->job_id)
         return 0;
         else if(job1->job_id > job2->job_id)
         return 1;
         else 
         return -1;
     }  


void *identity_copy(void *data)
{
    return data;
}



