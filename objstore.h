#ifndef OBJSTORE_H
#define OBJSTORE_H

#include "common.h"

struct objstore_t 
{
    char id_string[IDLEN];               // This must be a null terminated ASCII string
    struct timeval lastUpdateTime;
    int a;
    int b;
    vclock_t vclock[MAX_NODES];
};

typedef struct objstore_t objstore_t;

void objstore_init(objstore_t** store_ptr, const char* dataFileName);
void objstore_close(objstore_t** store_ptr);
void objstore_sync(objstore_t* store, vclock_t* vclock);
void objstore_dump(objstore_t* store);

int objstore_get_a(objstore_t* store);
int objstore_get_b(objstore_t* store);
const char* objstore_get_id(objstore_t* store);

void objstore_set_a(objstore_t* store, int new_a);
void objstore_set_b(objstore_t* store, int new_b);
void objstore_set_id(objstore_t* store, const char* new_id);

#endif
