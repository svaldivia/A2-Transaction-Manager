#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "objstore.h"

void objstore_init(objstore_t** store_ptr, const char* dataFileName) 
{
    assert(store_ptr != NULL);
    *store_ptr = NULL;

    int fd = 0;
    objstore_t* store = NULL;

    fd = open(dataFileName, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR );
    if (fd < 0) {
        printf("Could not open data file '%s'\n", dataFileName);
        exit(-1);
    }

    struct stat fstatus;
    if(fstat(fd, &fstatus) < 0) {
        printf("Filestat failed");
        exit(-2);
    }

    if (fstatus.st_size < sizeof(objstore_t)) 
    {
        /* File hasn's been mapped in before so we need to make sure there is enough
        space used in the file to hold the data.  */
        objstore_t space;
        memset(&space, 0, sizeof(objstore_t));
        gettimeofday(&space.lastUpdateTime, NULL);

        int rv = write(fd, &space, sizeof(objstore_t));
        if (rv != sizeof(objstore_t)) {
            printf("Some sort of writing error\n");
            exit(-3);
        }
        printf("Store file created\n");
    }

    /* Map to memory */
    store = (objstore_t*)mmap(NULL, 512, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

    if (store == NULL) {
        printf("Object data could not be mapped to memory\n");
        exit(-1);
    }

    *store_ptr = store;
}
  
void objstore_sync(objstore_t* store, vclock_t* vclock)
{
    assert(store);

    /* Set last update time */
    gettimeofday(&store->lastUpdateTime, NULL);

    /* copy vector clock */
    memcpy(store->vclock, vclock, MAX_NODES * sizeof(vclock_t));

    /* Flush to disc */
    int r = msync(store, sizeof(objstore_t), MS_SYNC);
    if (r != 0) {
        printf("Unable to sync data file\n");
        exit(-1);
    }
}

int objstore_get_a(objstore_t* store) { return store->a; }
void objstore_set_a(objstore_t* store, int new_a) {
    store->a = new_a;
}

int objstore_get_b(objstore_t* store) { return store->b; }
void objstore_set_b(objstore_t* store, int new_b) {
    store->b = new_b;
}

const char* objstore_get_id(objstore_t* store) { return store->id_string; }
void objstore_set_id(objstore_t* store, const char* new_id) {
    strncpy(store->id_string, new_id, IDLEN);
}

void objstore_close(objstore_t** store_ptr) 
{
    assert(store_ptr != NULL);
    objstore_t* store = *store_ptr;

    int rv = munmap(store, sizeof(objstore_t));
    if (rv < 0) {
        printf("Unmap failed");
        exit(-1);
    }

    *store_ptr = NULL;
}

void objstore_dump(objstore_t* store) 
{
    printf("Object file dump:\n");

    printf("  A  = %d\n", objstore_get_a(store));
    printf("  B  = %d\n", objstore_get_b(store));
    printf("  ID = '%s'\n", objstore_get_id(store));

    printf("Vector clock:\n");
    vclock_dump(store->vclock);

    char time_str[32];
    struct tm* ltime = localtime(&store->lastUpdateTime.tv_sec);
    strftime(time_str, 32, "%Y-%m-%d %H:%M:%S", ltime);
    printf("Last written at %s\n", time_str);
}
