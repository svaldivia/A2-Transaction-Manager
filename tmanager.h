#ifndef TMANAGER_H
#define TMANAGER_H

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "server.h"

#define MAX_TRANSACTIONS (4)

/* Stores information about an ongoing transaction */
struct transaction_t {
    uint32_t tid;
    uint32_t nodes[MAX_NODES];
};

typedef struct transaction_t transaction_t;

struct txmanager_t 
{
    server_t*     server;
    FILE*         logfile;
    vclock_t      vclock[MAX_NODES];
    transaction_t transactions[MAX_TRANSACTIONS];
    uint32_t      port;
};

typedef struct txmanager_t txmanager_t;

#endif
