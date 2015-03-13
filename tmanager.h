#ifndef TMANAGER_H
#define TMANAGER_H

#include <stdint.h>

#include "common.h"

#define MAX_TRANSACTIONS (4)

/* Stores information about an ongoing transaction */
struct transaction_t {
    uint32_t tid;
    uint32_t nodes[MAX_NODES];
};

typedef struct transaction_t transaction_t;

struct txmanager_t {
    uint32_t      port;
    vclock_t      vclock[MAX_NODES];
    transaction_t transactions[MAX_TRANSACTIONS];
};

typedef struct txmanager_t txmanager_t;

#endif
