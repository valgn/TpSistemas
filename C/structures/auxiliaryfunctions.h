#ifndef ___AUXILIARY_FUNCTION_H___
#define ___AUXILIARY_FUNCTION_H___

#include "data_types.h"

void* copy_request(void* source);

// void* p_copy_request(void* source);

void destroy_request(void* source);

// unsigned hash_request(void* source );

// unsigned comp_request(void* source1, void* source2 );

void *identity_copy(void *data);

#endif