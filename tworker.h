#ifndef TWORKER_H
#define TWORKER_H 1

#include <sys/time.h>
#include "common.h"

typedef     /* got the port number create a logfile name */
struct {
  char IDstring[IDLEN];   // This must be a null terminated ASCII string
  struct timeval lastUpdateTime;
  int A;
  int B;
  vclock_t vectorClock[MAX_NODES];
} ObjectData;

#endif /* TWORKER_H */
