#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>

int resolve_host(char* host, struct in_addr* out_addr)
{
    int error;
    struct addrinfo* result;
    struct sockaddr_in* addr;
    struct addrinfo hints;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    error = getaddrinfo(host, NULL, &hints, &result);
    if (error != 0) {
        /* blow up */
        fprintf(stderr, "Failed resolving hostname %s\n", host);
        return -1;
    }

    /* copy address to result */
    addr = (struct sockaddr_in*)result->ai_addr;
    memcpy(out_addr, &addr->sin_addr, sizeof(struct in_addr));

    freeaddrinfo(result);
    return 0;
}

void vclock_init(vclock_t* vclock) 
{
    assert(vclock != NULL);

    memset((void*)vclock, 0, sizeof(vclock_t));
}

void vclock_update(uint32_t my_id, vclock_t* mine, vclock_t* other)
{
    assert(mine != NULL);
    assert(other != NULL);

    /* For each other clock... */
    int i;
    for(i = 0; i < MAX_NODES; i++) 
    {
        /* Dont update empty nodes or our clock */
        if (other[i].nodeId == 0 || other[i].nodeId == my_id)
            continue;

        /* Find and update (or add if neccessary) our local version */
        int firstEmpty = -1;
        int index = -1;
        int j;
        for(j = 0; j < MAX_NODES; j++) 
        {
            if (mine[j].nodeId == 0)
                firstEmpty = j;

            if (mine[j].nodeId == other[i].nodeId) {
                index = j;
                break;
            }
        }

        /* It exists! */
        if (index > 0) {
            if (mine[index].time < other[i].time)
                mine[index].time = other[i].time;
            continue;
        }

        /* Does not yet exist but we have room for it in our array */
        if (firstEmpty > 0) {
            mine[firstEmpty] = other[i];
            continue;
        }

        printf("Cannot update vector clock for node %d: Maximum nodes reached\n", other[i].nodeId);
    }
} 

void vclock_increment(uint32_t my_id, vclock_t* vclock) {
    int c;
    for (c = 0; c < MAX_NODES; c++) {
        vclock_t* vc = &vclock[c];
        if (vc->nodeId == my_id) {
            vc->time++;
            return;
        }
    }
    printf("Could not increment vector clock, node ID %d doesn't exist\n", my_id);
    exit(1);
}

void vclock_dump(vclock_t* vclock) {
    int c;
    for (c = 0; c < MAX_NODES; c++)
        printf("  node[%d] = %d\n", vclock[c].nodeId, vclock[c].time);
}
