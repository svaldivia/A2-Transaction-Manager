#ifndef TWORKER_H
#define TWORKER_H 1

#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

typedef     /* got the port number create a logfile name */
struct {
  char IDstring[IDLEN];   // This must be a null terminated ASCII string
  struct timeval lastUpdateTime;
  int A;
  int B;
  vclock_t vectorClock[MAX_NODES];

} ObjectData;

struct worker_state_t 
{
    uint32_t tm_port;
    char     tm_host[64];

    /* wat do we need */
    bool do_commit;
    bool do_abort;
};

typedef struct worker_state_t worker_state_t;

void worker_thread(void* params);

#endif /* TWORKER_H */
