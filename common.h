#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_NODES 10
#define HOSTLEN   64
#define IDLEN     64

/* resolve the given hostname and store the address in the node struct */
int resolve_host(char* hostname, struct in_addr* out_addr);

struct vclock_t {
  unsigned int nodeId;
  unsigned int time;
};

typedef struct vclock_t vclock_t;

void vclock_init(vclock_t* vclock);
void vclock_update(uint32_t my_id, vclock_t* mine, vclock_t* other);
void vclock_increment(uint32_t my_id, vclock_t* vclock);
void vclock_dump(vclock_t* vclock);

#endif
